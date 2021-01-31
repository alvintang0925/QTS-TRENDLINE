//
//  main.cpp
//  QTS TRENDLINE
//
//  Created by 唐健恆 on 2020/11/24.
//  Copyright © 2020 Alvin. All rights reserved.
//

#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <string>
#include <vector>
#include <cstdlib>
#include <iomanip>
#include <cmath>
#include <ctime>
#include <cfloat>
#include <Eigen/Dense>
using namespace Eigen;
using namespace std;
using namespace __fs::filesystem;
#define FUNDS 10000000
#define PORTFOLIONUMBER 10
#define DELTA 0.0004
#define RUNTIMES 10000
#define EXPERIMENTNUMBER 50
#define QTSTYPE 2 //QTS 0, GQTS 1, GNQTS 2
#define TRENDLINETYPE 1 //linear 0, quadratic 1
#define SLIDETYPE 9 //M2M 0, Q2M 1, Q2Q 2, H2M 3, H2Q 4, H2H 5, Y2M 6, Y2Q 7, Y2H 8, Y2Y 9, M# 10, Q# 11, H# 12
string MODE = "test";
string TYPE;
string STARTYEAR = "2010";
string STARTMONTH = "1";
string ENDYEAR = "2019";
string ENDMONTH = "1";
string fileDir = "myOutput2";
int count_curve[3] = { 0 };
int train_range;
double current_funds = FUNDS;


double* beta;
vector < vector<string> > myData;

class Stock {
public:
    string company_name;
    int idx;
    double* price_list;
    void init(int);
    //    ~Stock();
};

//Stock::~Stock(){
//    delete[] price_list;
//}


void Stock::init(int size) {
    price_list = new double[size];
}

class Portfolio {
public:
    int* data;
    int* investment_number;
    double* total_money;
    double* remain_fund;
    double m;
    double a;
    double b;
    double daily_risk = 0;
    double trend = 0;
    Stock* constituent_stocks;
    double remain_money = 0;
    int stock_number = 0;
    int size = 0;
    int day_number = 0;
    double funds = 0;

    void init(int, int);
    int getDMoney();
    double getRemainMoney();
    double getNormalY(int);
    double getQuadraticY(int);
    void copyP(Portfolio&);
    Portfolio();
    ~Portfolio();
};

Portfolio::Portfolio() {

}

Portfolio::~Portfolio() {
    delete[] data;
    delete[] investment_number;
    delete[] constituent_stocks;
    delete[] remain_fund;
    delete[] total_money;
}

void Portfolio::init(int size, int day_number) {
    this -> funds = FUNDS;
    this->size = size;
    this->day_number = day_number;
    data = new int[size];
    for (int j = 0; j < size; j++) {
        data[j] = 0;
    }
    investment_number = new int[size];
    constituent_stocks = new Stock[size];
    remain_fund = new double[size];
    total_money = new double[day_number];
}

int Portfolio::getDMoney() {
    if (this->stock_number != 0) {
        int temp = this -> funds;
        return temp / this->stock_number;
    }
    else {
        return this -> funds;
    }
}

double Portfolio::getRemainMoney() {
    if (this->stock_number != 0) {
        double sum = 0;
        for (int j = 0; j < stock_number; j++) {
            sum += (double)this->getDMoney() - (this->investment_number[j] * this->constituent_stocks[j].price_list[0]);
        }
        int temp = this -> funds;
        return (temp % this->stock_number) + sum + (this -> funds - temp);
    }
    else {
        return this -> funds;
    }
}

double Portfolio::getNormalY(int day) {
    return this->m * day + this -> funds;
}

double Portfolio::getQuadraticY(int day) {
    return this->a * pow(day, 2) + this->b * day + this -> funds;
}

void Portfolio::copyP(Portfolio& a) {
    delete[](this->data);
    delete[](this->investment_number);
    delete[](this->total_money);
    delete[](this->remain_fund);
    delete[](this->constituent_stocks);
    this->data = new int[a.size];
    this->investment_number = new int[a.size];
    this->total_money = new double[a.day_number];
    this->remain_fund = new double[a.size];
    this->constituent_stocks = new Stock[a.size];
    for (int j = 0; j < a.size; j++) {
        this->data[j] = a.data[j];
    }
    for (int j = 0; j < a.stock_number; j++) {
        this->investment_number[j] = a.investment_number[j];
        this->constituent_stocks[j] = a.constituent_stocks[j];
        this->remain_fund[j] = a.remain_fund[j];
    }
    for (int j = 0; j < a.day_number; j++) {
        this->total_money[j] = a.total_money[j];
    }
    this->m = a.m;
    this->a = a.a;
    this->b = a.b;
    this->daily_risk = a.daily_risk;
    this->trend = a.trend;
    this->remain_money = a.remain_money;
    this->stock_number = a.stock_number;
    this->size = a.size;
    this->day_number = a.day_number;
}

class Date {
public:
    tm date = { 0 };
    int slide_numer;
    string getYear();
    string getQ();
    string getMon();
    Date getRangeEnd(int);
    void slide();
};



string Date::getYear() {
    return to_string((this->date.tm_year) + 1900);
}

string Date::getQ() {
    return "Q" + to_string((this->date.tm_mon / 3) + 1);
}

string Date::getMon() {
    if (this->date.tm_mon > 8) {
        return to_string((this->date.tm_mon) + 1);
    }
    else {
        return "0" + to_string((this->date.tm_mon) + 1);
    }
}

Date Date::getRangeEnd(int range) {
    tm temp = this->date;
    temp.tm_mon += range;
    time_t temp2 = mktime(&temp);
    tm* temp3 = localtime(&temp2);
    Date temp4;
    temp4.date = *temp3;
    return temp4;
}

void Date::slide() {
    tm temp = this->date;
    temp.tm_mon += slide_numer;
    time_t temp2 = mktime(&temp);
    tm* temp3 = localtime(&temp2);
    this->date = *temp3;

}

void countQuadraticYLine(Portfolio& portfolio) {
    MatrixXd A(portfolio.day_number, 2);
    VectorXd Y(portfolio.day_number, 1);
    for (int j = 0; j < portfolio.day_number; j++) {
        for (int k = 0; k < 2; k++) {
            A(j, k) = pow(j + 1, 2 - k);
        }
        Y(j, 0) = portfolio.total_money[j] - portfolio.funds;
    }
    Vector2d theta = A.colPivHouseholderQr().solve(Y);
    portfolio.a = theta(0, 0);
    portfolio.b = theta(1, 0);
}


Portfolio gBest;
Portfolio gWorst;
Portfolio pBest;
Portfolio pWorst;
Portfolio expBest;
int g_gen;
int answer_gen;
int answer_exp;
int answer_counter = 1;


vector< vector<string> > readData(string filename) {
    cout << filename << endl;
    ifstream inFile(filename, ios::in);
    string line;
    vector< vector<string> > data;

    if (!inFile) {
        cout << "Open file failed!" << endl;
        exit(1);
    }
    while (getline(inFile, line)) {
        istringstream delime(line);
        string s;
        vector<string> line_data;
        while (getline(delime, s, ',')) {
            if (s != "\r") {
                line_data.push_back(s);
            }
        }
        data.push_back(line_data);
    }
    inFile.close();
    return data;
}

vector<string> readSpeData(string filename) {
    cout << filename << endl;
    ifstream inFile(filename, ios::in);
    string line;
    vector< vector<string> > data;

    if (!inFile) {
        cout << "Open file failed!" << endl;
        exit(1);
    }
    bool sw = false;
    vector<string> line_data;
    while(getline(inFile, line)){
        istringstream delime(line);
        string s;
        
        while(getline(delime, s, ',')){
            if(sw){
                //s.pop_back();
                if(s != "\r"){
                    s.erase(remove(s.begin(), s.end(), '\r'), s.end());
                    line_data.push_back(s);
                }
                //sw = false;
            }
            if(s == "Stock#"){
                sw = true;
            }
        }
        sw = false;
    }
    inFile.close();
    return line_data;
}

void createStock(Stock* stock_list) {
    for (int j = 0; j < myData[0].size(); j++) {
        stock_list[j].idx = j;
        stock_list[j].init(myData.size() - 1);

        for (int k = 0; k < myData.size(); k++) {
            if (k == 0) {
                stock_list[j].company_name = myData[k][j];
            }
            else {
                stock_list[j].price_list[k - 1] = atof(myData[k][j].c_str());
            }
        }
    }
}

void initial() {
    gBest.trend = 0;
    gWorst.trend = DBL_MAX;
    gBest.init(myData[0].size(), myData.size());
    beta = new double[myData[0].size()];
    for (int j = 0; j < myData[0].size(); j++) {
        beta[j] = 0.5;
    }
}

void gen_portfolio(Portfolio* portfolio_list, Stock* stock_list, int portfolio_number, string mode) {
    for (int j = 0; j < portfolio_number; j++) {
        portfolio_list[j].stock_number = 0;
        if(mode == "train"){
            portfolio_list[j].init(myData[0].size(), myData.size() - 1);
            for (int k = 0; k < myData[0].size(); k++) {

                double r = (double)rand() / (double)RAND_MAX;
                if (r > beta[k]) {
                    portfolio_list[j].data[k] = 0;
                }
                else {
                    portfolio_list[j].data[k] = 1;
                }
            }
        }
        
        for(int k = 0; k < myData[0].size(); k++){
            if(portfolio_list[j].data[k] == 1){
                portfolio_list[j].constituent_stocks[portfolio_list[j].stock_number] = stock_list[k];
                portfolio_list[j].stock_number++;
            }
        }
    }
}

void gen_testPortfolio(Portfolio* portfolio_list, Stock* stock_list, int portfolio_number, string mode, vector<string>myTrainData) {
    for (int j = 0; j < portfolio_number; j++) {
        portfolio_list[j].stock_number = 0;
        for(int k = 0; k < myData[0].size(); k++){
            for(int h = 0; h < myTrainData.size(); h++){
                if(myData[0][k] == myTrainData[h]){
                    portfolio_list[j].data[k] = 1;
                    portfolio_list[j].constituent_stocks[portfolio_list[j].stock_number] = stock_list[k];
                    portfolio_list[j].stock_number++;
                    break;
                }
            }
        }
    }
}

void capitalLevel(Portfolio* portfolio_list, int portfolio_number, double funds) {
    for (int j = 0; j < portfolio_number; j++) {

        for (int k = 0; k < portfolio_list[j].stock_number; k++) {
            portfolio_list[j].investment_number[k] = portfolio_list[j].getDMoney() / portfolio_list[j].constituent_stocks[k].price_list[0];
            portfolio_list[j].remain_fund[k] = portfolio_list[j].getDMoney() - (portfolio_list[j].investment_number[k] * portfolio_list[j].constituent_stocks[k].price_list[0]);
        }
        portfolio_list[j].total_money[0] = funds;

    }

    for (int j = 0; j < myData.size() - 2; j++) {
        for (int k = 0; k < portfolio_number; k++) {
            portfolio_list[k].total_money[j + 1] = portfolio_list[k].getRemainMoney();
            for (int h = 0; h < portfolio_list[k].stock_number; h++) {
                portfolio_list[k].total_money[j + 1] += portfolio_list[k].investment_number[h] * portfolio_list[k].constituent_stocks[h].price_list[j + 1];
            }
        }
    }
}

void countTrend(Portfolio* portfolio_list, int porfolio_number, double funds) {
    for (int j = 0; j < porfolio_number; j++) {
        double sum = 0;
        if (TRENDLINETYPE == 0) {
            double x = 0;
            double y = 0;
            for (int k = 0; k < portfolio_list[j].day_number; k++) {
                x += (k + 1) * (portfolio_list[j].total_money[k] - funds);
                y += (k + 1) * (k + 1);
            }
            if (portfolio_list[j].stock_number != 0) {
                portfolio_list[j].m = x / y;
            }
            for (int k = 0; k < portfolio_list[j].day_number; k++) {
                double Y;
                Y = portfolio_list[j].getNormalY(k + 1);
                sum += (portfolio_list[j].total_money[k] - Y) * (portfolio_list[j].total_money[k] - Y);
            }
        }
        else if (TRENDLINETYPE == 1) {
            countQuadraticYLine(portfolio_list[j]);
            for (int k = 0; k < portfolio_list[j].day_number; k++) {
                double Y;
                Y = portfolio_list[j].getQuadraticY(k + 1);
                sum += (portfolio_list[j].total_money[k] - Y) * (portfolio_list[j].total_money[k] - Y);
            }
            portfolio_list[j].m = (portfolio_list[j].getQuadraticY(portfolio_list[j].day_number) - portfolio_list[j].getQuadraticY(1)) / (portfolio_list[j].day_number - 1);
        }

        portfolio_list[j].daily_risk = sqrt(sum / (portfolio_list[j].day_number));

        if (portfolio_list[j].m < 0) {
            portfolio_list[j].trend = portfolio_list[j].m * portfolio_list[j].daily_risk;
        }
        else {
            portfolio_list[j].trend = portfolio_list[j].m / portfolio_list[j].daily_risk;
        }
    }
}

void recordGAnswer(Portfolio* portfolio_list, int i) {
    pBest.copyP(portfolio_list[0]);
    pWorst.copyP(portfolio_list[PORTFOLIONUMBER - 1]);
    for (int j = 0; j < PORTFOLIONUMBER; j++) {
        if (pBest.trend < portfolio_list[j].trend) {
            pBest.copyP(portfolio_list[j]);
        }
        if (pWorst.trend > portfolio_list[j].trend) {
            pWorst.copyP(portfolio_list[j]);
        }
    }

    if (gBest.trend < pBest.trend) {
        gBest.copyP(pBest);
        g_gen = i + 1;
    }

    if (gWorst.trend > pWorst.trend) {
        gWorst.copyP(pWorst);
    }
}

void adjBeta() {
    for (int j = 0; j < myData[0].size(); j++) {
        if (QTSTYPE == 2) {
            if (gBest.data[j] > pWorst.data[j]) {
                if (beta[j] < 0.5) {
                    beta[j] = 1 - beta[j];
                }
                beta[j] += DELTA;
            }
            else if (gBest.data[j] < pWorst.data[j]) {
                if (beta[j] > 0.5) {
                    beta[j] = 1 - beta[j];
                }
                beta[j] -= DELTA;
            }
        }
        else if (QTSTYPE == 1) {
            if (gBest.data[j] > pWorst.data[j]) {
                beta[j] += DELTA;
            }
            else if (gBest.data[j] < pWorst.data[j]) {
                beta[j] -= DELTA;
            }
        }
        else {
            if (pBest.data[j] > pWorst.data[j]) {
                beta[j] += DELTA;
            }
            else if (pBest.data[j] < pWorst.data[j]) {
                beta[j] -= DELTA;
            }
        }
    }
}

void recordExpAnswer(int n) {
    if (expBest.trend < gBest.trend) {
        expBest.copyP(gBest);
        answer_gen = g_gen;
        answer_exp = n + 1;
        answer_counter = 1;
    }
    else if (expBest.trend == gBest.trend) {
        answer_counter++;
    }
}

string getOutputFilename(Date current_date, string mode){
    return fileDir + "/" + TYPE + "/" + mode + "/" + mode + "_" + current_date.getYear() + "_" + current_date.getMon() + ".csv";
}

void outputFile(Date current_date, Portfolio& portfolio, string mode) {
    ofstream outfile;
    string file_name = getOutputFilename(current_date, mode);
    outfile.open(file_name, ios::out);
    outfile << fixed << setprecision(10);


    outfile << "Exp times: ," << EXPERIMENTNUMBER << endl;
    outfile << "Iteration: ," << RUNTIMES << endl;
    outfile << "Element number: ," << PORTFOLIONUMBER << endl;
    outfile << "Delta: ," << DELTA << endl;
    outfile << "Init funds: ," << portfolio.funds << endl;
    outfile << "Final funds: ," << portfolio.total_money[myData.size() - 2] << endl;
    outfile << "Real award: ," << portfolio.total_money[myData.size() - 2] - portfolio.funds << endl << endl;
    outfile << "m: ," << portfolio.m << endl;
    outfile << "Daily_risk: ," << portfolio.daily_risk << endl;
    outfile << "Trend: ," << portfolio.trend << endl << endl;

    if (TRENDLINETYPE == 0) {
        countQuadraticYLine(portfolio);
        double sum = 0;
        for (int k = 0; k < myData.size() - 1; k++) {
            double Y;
            Y = portfolio.getQuadraticY(k + 1);
            sum += (portfolio.total_money[k] - Y) * (portfolio.total_money[k] - Y);
        }
        double c = (portfolio.getQuadraticY(myData.size() - 1) - portfolio.getQuadraticY(1)) / (myData.size() - 2);
        double d = sqrt(sum / (myData.size() - 1));

        outfile << "Quadratic m:," << c << endl;
        outfile << "Quadratic daily risk:," << d << endl;
        if(c < 0){
            outfile << "Quadratic trend:," << c * d << endl << endl;
        }else{
            outfile << "Quadratic trend:," << c / d << endl << endl;
        }
    }
    else {
        outfile << "Quadratic trend line:," << portfolio.a << "x^2 + " << portfolio.b << "x + " << FUNDS << endl << endl;
        double x = 0;
        double y = 1;
        double sum = 0;
        for (int k = 0; k < myData.size() - 2; k++) {
            x += (k + 2) * (portfolio.total_money[k + 1] - portfolio.funds);
            y += (k + 2) * (k + 2);
        }

        double c = x / y;
        for (int k = 0; k < myData.size() - 1; k++) {
            double Y;
            Y = c * (k + 1) + portfolio.funds;
            sum += (portfolio.total_money[k] - Y) * (portfolio.total_money[k] - Y);
        }
        double d = sqrt(sum / (myData.size() - 1));

        outfile << "Linear m:," << c << endl;
        outfile << "Linear daily risk:," << d << endl;
        if(c < 0){
            outfile << "Linear trend:," << c * d << endl << endl;
        }else{
            outfile << "Linear trend:," << c / d << endl << endl;
        }
    }

    outfile << "Best generation," << answer_gen << endl;
    outfile << "Best experiment," << answer_exp << endl;
    outfile << "Best answer times," << answer_counter << endl << endl;

    outfile << "Stock number: ," << portfolio.stock_number << endl;
    outfile << "Stock#,";
    for (int j = 0; j < portfolio.stock_number; j++) {
        outfile << portfolio.constituent_stocks[j].company_name << ",";
    }
    outfile << endl;

    outfile << "Number: ,";
    for (int j = 0; j < portfolio.stock_number; j++) {
        outfile << portfolio.investment_number[j] << ",";
    }
    outfile << endl;

    outfile << "Distribue funds: ,";
    for (int j = 0; j < portfolio.stock_number; j++) {
        outfile << portfolio.getDMoney() << ",";
    }
    outfile << endl;

    outfile << "Remain funds: ,";
    for (int j = 0; j < portfolio.stock_number; j++) {
        outfile << portfolio.remain_fund[j] << ",";
    }
    outfile << endl;

    for (int j = 0; j < myData.size() - 1; j++) {
        outfile << "FS(" << j + 1 << "),";
        for (int k = 0; k < portfolio.stock_number; k++) {
            outfile << portfolio.constituent_stocks[k].price_list[j] * portfolio.investment_number[k] << ",";
        }
        outfile << portfolio.total_money[j] << endl;
    }
    outfile << endl;
    outfile.close();

}

bool isFinish(tm current_data, tm finish_date) {
    if (current_data.tm_year > finish_date.tm_year) {
        return true;
    }
    if ((current_data.tm_year == finish_date.tm_year) && (current_data.tm_mon > finish_date.tm_mon)) {
        return true;
    }
    return false;
}
string getPriceFilename(Date current_date, string mode) {
    Date temp;
    if(mode == "train"){
        switch (SLIDETYPE) {
        case 0://M2M
            TYPE = "M2M";
                return "DJI_30/M2M/" + mode + "_" + current_date.getYear() + "_" + current_date.getMon() + "(" + current_date.getYear() + " Q1).csv";
            break;
        case 1://Q2M
            TYPE = "Q2M";
            temp = current_date.getRangeEnd(2);
            if (current_date.getYear() != temp.getYear()) {
                return "DJI_30/Q2M/" + mode + "_" + current_date.getYear() + "_" + current_date.getMon() + "~" + temp.getYear() + "_" + temp.getMon() + "(" + current_date.getYear() + " Q1).csv";
            }
            else {
                return "DJI_30/Q2M/" + mode + "_" + current_date.getYear() + "_" + current_date.getMon() + "-" + temp.getMon() + "(" + current_date.getYear() + " Q1).csv";
            }
            break;
        case 2://Q2Q
            TYPE = "Q2Q";
            return "DJI_30/Q2Q/" + mode + "_" + current_date.getYear() + "_" + current_date.getQ() + "(" + current_date.getYear() + " Q1).csv";
            break;
        case 3://H2M
            TYPE = "H2M";
            temp = current_date.getRangeEnd(5);
            if (current_date.getYear() != temp.getYear()) {
                return "DJI_30/H2M/" + mode + "_" + current_date.getYear() + "_" + current_date.getMon() + "~" + temp.getYear() + "_" + temp.getMon() + "(" + current_date.getYear() + " Q1).csv";
            }
            else {
                return "DJI_30/H2M/" + mode + "_" + current_date.getYear() + "_" + current_date.getMon() + "-" + temp.getMon() + "(" + current_date.getYear() + " Q1).csv";
            }
            break;
        case 4://H2Q
            TYPE = "H2Q";
            temp = current_date.getRangeEnd(5);
            if (current_date.getYear() != temp.getYear()) {
                return "DJI_30/H2Q/" + mode + "_" + current_date.getYear() + "_" + current_date.getQ() + "~" + temp.getYear() + "_" + temp.getQ() + "(" + current_date.getYear() + " Q1).csv";
            }
            else {
                return "DJI_30/H2Q/" + mode + "_" + current_date.getYear() + "_" + current_date.getQ() + "-" + temp.getQ() + "(" + current_date.getYear() + " Q1).csv";
            }
            break;
        case 5://H2H
            TYPE = "H2H";
            temp = current_date.getRangeEnd(5);
            return "DJI_30/H2H/" + mode + "_" + current_date.getYear() + "_" + current_date.getQ() + "-" + temp.getQ() + "(" + current_date.getYear() + " Q1).csv";
            break;
        case 6://Y2M
            TYPE = "Y2M";
            temp = current_date.getRangeEnd(11);
            if (current_date.getYear() != temp.getYear()) {
                return "DJI_30/Y2M/" + mode + "_" + current_date.getYear() + "_" + current_date.getMon() + "~" + temp.getYear() + "_" + temp.getMon() + "(" + current_date.getYear() + " Q1).csv";
            }
            else {
                return "DJI_30/Y2M/" + mode + "_" + current_date.getYear() + "(" + current_date.getYear() + " Q1).csv";
            }
            break;
        case 7://Y2Q
            TYPE = "Y2Q";
            temp = current_date.getRangeEnd(11);
            if (current_date.getYear() != temp.getYear()) {
                return "DJI_30/Y2Q/" + mode + "_" + current_date.getYear() + "_" + current_date.getQ() + "~" + temp.getYear() + "_" + temp.getQ() + "(" + current_date.getYear() + " Q1).csv";
            }
            else {
                return "DJI_30/Y2Q/" + mode + "_" + current_date.getYear() + "(" + current_date.getYear() + " Q1).csv";
            }
            break;
        case 8://Y2H
            TYPE = "Y2H";
            temp = current_date.getRangeEnd(11);
            if (current_date.getYear() != temp.getYear()) {
                return "DJI_30/Y2H/" + mode + "_" + current_date.getYear() + "_" + current_date.getQ() + "~" + temp.getYear() + "_" + temp.getQ() + "(" + current_date.getYear() + " Q1).csv";
            }
            else {
                return "DJI_30/Y2H/" + mode + "_" + current_date.getYear() + "(" + current_date.getYear() + " Q1).csv";
            }
            break;
        case 9://Y2Y
            TYPE = "Y2Y";
            return "DJI_30/Y2Y/" + mode + "_" + current_date.getYear() + "(" + current_date.getYear() + " Q1).csv";
            break;
        case 10://M#
            TYPE = "M#";
            return "DJI_30/M#/" + mode + "_" + to_string(atoi(current_date.getYear().c_str()) - 1) + "_" + current_date.getMon() + "(" + to_string(atoi(current_date.getYear().c_str()) - 1) + " Q1).csv";
            break;
        case 11://Q#
            TYPE = "Q#";
            return "DJI_30/Q#/" + mode + "_" + to_string(atoi(current_date.getYear().c_str()) - 1) + "_" + current_date.getQ() + "(" + to_string(atoi(current_date.getYear().c_str()) - 1) + " Q1).csv";
            break;
        case 12://H#
            TYPE = "H#";
            temp = current_date.getRangeEnd(5);
            return "DJI_30/H#/" + mode + "_" + to_string(atoi(current_date.getYear().c_str()) - 1) + "_" + current_date.getQ() + "-" + temp.getQ() + "(" + to_string(atoi(current_date.getYear().c_str()) - 1) + " Q1).csv";
            break;
        }
    }else{
        switch (SLIDETYPE) {
        case 0://M2M
            TYPE = "M2M";
                return "DJI_30/M2M/" + mode + "_" + current_date.getYear() + "_" + current_date.getMon() + "(" + current_date.getRangeEnd(-1 * train_range).getYear() + " Q1).csv";
            break;
        case 1://Q2M
            TYPE = "Q2M";
            temp = current_date.getRangeEnd(2);
            if (current_date.getYear() != temp.getYear()) {
                return "DJI_30/Q2M/" + mode + "_" + current_date.getYear() + "_" + current_date.getMon() + "~" + temp.getYear() + "_" + temp.getMon() + "(" + current_date.getRangeEnd(-1 * train_range).getYear() + " Q1).csv";
            }
            else {
                return "DJI_30/Q2M/" + mode + "_" + current_date.getYear() + "_" + current_date.getMon() + "-" + temp.getMon() + "(" + current_date.getRangeEnd(-1 * train_range).getYear() + " Q1).csv";
            }
            break;
        case 2://Q2Q
            TYPE = "Q2Q";
            return "DJI_30/Q2Q/" + mode + "_" + current_date.getYear() + "_" + current_date.getQ() + "(" + current_date.getRangeEnd(-1 * train_range).getYear() + " Q1).csv";
            break;
        case 3://H2M
            TYPE = "H2M";
            temp = current_date.getRangeEnd(5);
            if (current_date.getYear() != temp.getYear()) {
                return "DJI_30/H2M/" + mode + "_" + current_date.getYear() + "_" + current_date.getMon() + "~" + temp.getYear() + "_" + temp.getMon() + "(" + current_date.getRangeEnd(-1 * train_range).getYear() + " Q1).csv";
            }
            else {
                return "DJI_30/H2M/" + mode + "_" + current_date.getYear() + "_" + current_date.getMon() + "-" + temp.getMon() + "(" + current_date.getRangeEnd(-1 * train_range).getYear() + " Q1).csv";
            }
            break;
        case 4://H2Q
            TYPE = "H2Q";
            temp = current_date.getRangeEnd(5);
            if (current_date.getYear() != temp.getYear()) {
                return "DJI_30/H2Q/" + mode + "_" + current_date.getYear() + "_" + current_date.getQ() + "~" + temp.getYear() + "_" + temp.getQ() + "(" + current_date.getRangeEnd(-1 * train_range).getYear() + " Q1).csv";
            }
            else {
                return "DJI_30/H2Q/" + mode + "_" + current_date.getYear() + "_" + current_date.getQ() + "-" + temp.getQ() + "(" + current_date.getRangeEnd(-1 * train_range).getYear() + " Q1).csv";
            }
            break;
        case 5://H2H
            TYPE = "H2H";
            temp = current_date.getRangeEnd(5);
            return "DJI_30/H2H/" + mode + "_" + current_date.getYear() + "_" + current_date.getQ() + "-" + temp.getQ() + "(" + current_date.getRangeEnd(-1 * train_range).getYear() + " Q1).csv";
            break;
        case 6://Y2M
            TYPE = "Y2M";
            temp = current_date.getRangeEnd(11);
            if (current_date.getYear() != temp.getYear()) {
                return "DJI_30/Y2M/" + mode + "_" + current_date.getYear() + "_" + current_date.getMon() + "~" + temp.getYear() + "_" + temp.getMon() + "(" + current_date.getRangeEnd(-1 * train_range).getYear() + " Q1).csv";
            }
            else {
                return "DJI_30/Y2M/" + mode + "_" + current_date.getYear() + "(" + current_date.getRangeEnd(-1 * train_range).getYear() + " Q1).csv";
            }
            break;
        case 7://Y2Q
            TYPE = "Y2Q";
            temp = current_date.getRangeEnd(11);
            if (current_date.getYear() != temp.getYear()) {
                return "DJI_30/Y2Q/" + mode + "_" + current_date.getYear() + "_" + current_date.getQ() + "~" + temp.getYear() + "_" + temp.getQ() + "(" + current_date.getRangeEnd(-1 * train_range).getYear() + " Q1).csv";
            }
            else {
                return "DJI_30/Y2Q/" + mode + "_" + current_date.getYear() + "(" + current_date.getRangeEnd(-1 * train_range).getYear() + " Q1).csv";
            }
            break;
        case 8://Y2H
            TYPE = "Y2H";
            temp = current_date.getRangeEnd(11);
            if (current_date.getYear() != temp.getYear()) {
                return "DJI_30/Y2H/" + mode + "_" + current_date.getYear() + "_" + current_date.getQ() + "~" + temp.getYear() + "_" + temp.getQ() + "(" + current_date.getRangeEnd(-1 * train_range).getYear() + " Q1).csv";
            }
            else {
                return "DJI_30/Y2H/" + mode + "_" + current_date.getYear() + "(" + current_date.getRangeEnd(-1 * train_range).getYear() + " Q1).csv";
            }
            break;
        case 9://Y2Y
            TYPE = "Y2Y";
            return "DJI_30/Y2Y/" + mode + "_" + current_date.getYear() + "(" + current_date.getRangeEnd(-1 * train_range).getYear() + " Q1).csv";
            break;
        case 10://M#
            TYPE = "M#";
            return "DJI_30/M#/" + mode + "_" + to_string(atoi(current_date.getYear().c_str()) - 1) + "_" + current_date.getMon() + "(" + to_string(atoi(current_date.getRangeEnd(-1 * train_range).getYear().c_str()) - 1) + " Q1).csv";
            break;
        case 11://Q#
            TYPE = "Q#";
            return "DJI_30/Q#/" + mode + "_" + to_string(atoi(current_date.getYear().c_str()) - 1) + "_" + current_date.getQ() + "(" + to_string(atoi(current_date.getRangeEnd(-1 * train_range).getYear().c_str()) - 1) + " Q1).csv";
            break;
        case 12://H#
            TYPE = "H#";
            temp = current_date.getRangeEnd(5);
            return "DJI_30/H#/" + mode + "_" + to_string(atoi(current_date.getYear().c_str()) - 1) + "_" + current_date.getQ() + "-" + temp.getQ() + "(" + to_string(atoi(current_date.getRangeEnd(-1 * train_range).getYear().c_str()) - 1) + " Q1).csv";
            break;
        }
    }
}


void setDate(Date& current_date, Date& finish_date) {
    current_date.date.tm_year = atoi(STARTYEAR.c_str()) - 1900;
    current_date.date.tm_mon = atoi(STARTMONTH.c_str()) - 1;
    current_date.date.tm_mday = 1;
    finish_date.date.tm_year = atoi(ENDYEAR.c_str()) - 1900;
    finish_date.date.tm_mon = atoi(ENDMONTH.c_str()) - 1;
    finish_date.date.tm_mday = 1;
    switch (SLIDETYPE) {
    case 0:
        TYPE = "M2M";
        train_range = 1;
        current_date.slide_numer = 1;
        break;
    case 1:
        TYPE = "Q2M";
        train_range = 3;
        current_date.slide_numer = 1;
        break;
    case 2:
        TYPE = "Q2Q";
        train_range = 3;
        current_date.slide_numer = 3;
        break;
    case 3:
        TYPE = "H2M";
        train_range = 6;
        current_date.slide_numer = 1;
        break;
    case 4:
        TYPE = "H2Q";
        train_range = 6;
        current_date.slide_numer = 3;
        break;
    case 5:
        TYPE = "H2H";
        train_range = 6;
        current_date.slide_numer = 6;
        break;
    case 6:
        TYPE = "Y2M";
        train_range = 12;
        current_date.slide_numer = 1;
        break;
    case 7:
        TYPE = "Y2Q";
        train_range = 12;
        current_date.slide_numer = 3;
        break;
    case 8:
        TYPE = "Y2H";
        train_range = 12;
        current_date.slide_numer = 6;
        break;
    case 9:
        TYPE = "Y2Y";
        train_range = 12;
        current_date.slide_numer = 12;
        break;
    case 10:
        TYPE = "M#";
        train_range = 1;
        current_date.slide_numer = 1;
        break;
    case 11:
        TYPE = "Q#";
        train_range = 3;
        current_date.slide_numer = 3;
        break;
    case 12:
        TYPE = "H#";
        train_range = 6;
        current_date.slide_numer = 6;
        break;
    }
    if(MODE == "train"){
        train_range = 0;
    }
}

void createDir(){
    create_directory(fileDir);
    create_directory(fileDir + "/" + TYPE);
    create_directory(fileDir + "/" + TYPE + "/train");
    create_directory(fileDir + "/" + TYPE + "/test");
//    create_directory(fileDir + "/Q2M");
//    create_directory(fileDir + "/Q2M/train");
//    create_directory(fileDir + "/Q2M/test");
//    create_directory(fileDir + "/H2M");
//    create_directory(fileDir + "/H2M/train");
//    create_directory(fileDir + "/H2M/test");
//    create_directory(fileDir + "/Y2M");
//    create_directory(fileDir + "/Y2M/train");
//    create_directory(fileDir + "/Y2M/test");
//    create_directory(fileDir + "/Q2Q");
//    create_directory(fileDir + "/Q2Q/train");
//    create_directory(fileDir + "/Q2Q/test");
//    create_directory(fileDir + "/H2Q");
//    create_directory(fileDir + "/H2Q/train");
//    create_directory(fileDir + "/H2Q/test");
//    create_directory(fileDir + "/Y2Q");
//    create_directory(fileDir + "/Y2Q/train");
//    create_directory(fileDir + "/Y2Q/test");
//    create_directory(fileDir + "/H2H");
//    create_directory(fileDir + "/H2H/train");
//    create_directory(fileDir + "/H2H/test");
//    create_directory(fileDir + "/Y2H");
//    create_directory(fileDir + "/Y2H/train");
//    create_directory(fileDir + "/Y2H/test");
//    create_directory(fileDir + "/Y2Y");
//    create_directory(fileDir + "/Y2Y/train");
//    create_directory(fileDir + "/Y2Y/test");
//    create_directory(fileDir + "/M#");
//    create_directory(fileDir + "/M#/train");
//    create_directory(fileDir + "/M#/test");
//    create_directory(fileDir + "/Q#");
//    create_directory(fileDir + "/Q#/train");
//    create_directory(fileDir + "/Q#/test");
//    create_directory(fileDir + "/H#");
//    create_directory(fileDir + "/H#/train");
//    create_directory(fileDir + "/H#/test");
}




int main(int argc, const char* argv[]) {
    double START, END;
    START = clock();

    srand(114);
    cout << fixed << setprecision(10);
    Date current_date, finish_date;
    setDate(current_date, finish_date);
    createDir();
    
    double out_m = 0;
    double out_daily_risk = 0;
    double out_trend = 0;
    double out_real_award = 0;
    double out_daily_award[250] = {0};
    int out_counter = 0;
    do {
        string filename;
        if(MODE == "train"){
            filename = getPriceFilename(current_date, MODE);
            myData = readData(filename);
            Stock* stock_list = new Stock[myData[0].size()];
            createStock(stock_list);
            expBest.trend = 0;
            expBest.init(myData[0].size(), myData.size());
            for (int n = 0; n < EXPERIMENTNUMBER; n++) {
                initial();
                for (int i = 0; i < RUNTIMES; i++) {
                    Portfolio* portfolio_list = new Portfolio[PORTFOLIONUMBER];
                    gen_portfolio(portfolio_list, stock_list, PORTFOLIONUMBER, "train");
                    capitalLevel(portfolio_list, PORTFOLIONUMBER, FUNDS);
                    countTrend(portfolio_list, PORTFOLIONUMBER, FUNDS);
                    //            countQuadraticYLine(portfolio_list);
                    recordGAnswer(portfolio_list, i);
                    adjBeta();
                    delete[] portfolio_list;
                }
                recordExpAnswer(n);
                cout << "___" << n << "___" << endl;
            }
            outputFile(current_date, expBest, "train");
            delete[] stock_list;
            cout << "exp: " << answer_exp << endl;
            cout << "gen: " << answer_gen << endl;
            cout << "m:" << expBest.m << endl;
            cout << "risk:" << expBest.daily_risk << endl;
            cout << "trend: " << expBest.trend << endl;
            if (expBest.a < 0) {
                count_curve[0]++;
            }
            else if (expBest.a > 0) {
                count_curve[2]++;
            }
            else {
                count_curve[1]++;
            }
        }
        
        
        
//        if(MODE == "test"){
//            filename = getPriceFilename(current_date, MODE);
//            myData = readData(filename);
//            Stock* test_stock_list = new Stock[myData[0].size()];
//            createStock(test_stock_list);
//            Portfolio* test_portfolio = new Portfolio[1];
//            test_portfolio[0].trend = 0;
//            test_portfolio[0].init(myData[0].size(), myData.size());
//            test_portfolio[0].copyP(expBest);
//            test_portfolio[0].day_number = myData.size() - 1;
//            gen_portfolio(test_portfolio, test_stock_list, 1, "test");
//            test_portfolio[0].funds = current_funds;
//            capitalLevel(test_portfolio, 1, current_funds);
//            countTrend(test_portfolio, 1, current_funds);
//            current_funds = test_portfolio[0].total_money[test_portfolio[0].day_number - 1];
//            outputFile(current_date, test_portfolio[0], "test");
//            delete[] test_portfolio;
//            delete[] test_stock_list;
//        }
        if(MODE == "test"){
            vector<string> myTrainData = readSpeData(getOutputFilename(current_date.getRangeEnd(-1 * train_range), "train"));
            filename = getPriceFilename(current_date, MODE);
            myData = readData(filename);
            Stock* test_stock_list = new Stock[myData[0].size()];
            createStock(test_stock_list);
            Portfolio* test_portfolio = new Portfolio[1];
            test_portfolio[0].init(myData[0].size(), myData.size()-1);
            gen_testPortfolio(test_portfolio, test_stock_list, 1, "test", myTrainData);
//            test_portfolio[0].funds = current_funds;
            
            capitalLevel(test_portfolio, 1, current_funds);
            countTrend(test_portfolio, 1, current_funds);
//            current_funds = test_portfolio[0].total_money[test_portfolio[0].day_number - 1];
            out_m += test_portfolio[0].m;
            out_daily_risk += test_portfolio[0].daily_risk;
            if(test_portfolio[0].trend < 0){
                out_trend += test_portfolio[0].m / test_portfolio[0].daily_risk;
            }else{
                out_trend += test_portfolio[0].trend;
            }
            out_real_award += test_portfolio[0].total_money[myData.size() - 2] - test_portfolio[0].funds;
            for(int j = 0; j < 250; j++){
                out_daily_award[j] += test_portfolio[0].total_money[j];
            }
            out_counter++;
            outputFile(current_date, test_portfolio[0], "test");
            delete[] test_portfolio;
            delete[] test_stock_list;
            
        }
        current_date.slide();
    } while (!isFinish(current_date.date, finish_date.date));
    END = clock();
    double finish_time = (END - START) / CLOCKS_PER_SEC;
    if(MODE == "train"){
        ofstream outfile_time;
        string file_name = fileDir + "/" + TYPE + "/" + "time.txt";
        outfile_time.open(file_name, ios::out);
        outfile_time << "total time: " << finish_time << " sec" << endl;
        outfile_time << "up: " << count_curve[2] << endl;
        outfile_time << "linear: " << count_curve[1] << endl;
        outfile_time << "down: " << count_curve[0] << endl;
        cout << "total_time: " << (END - START) / CLOCKS_PER_SEC << endl;
    }
//    if(MODE == "test"){
//        ofstream outfile_time;
//        string file_name = fileDir + "/" + TYPE + "/" + MODE + "/avgData.csv";
//        outfile_time.open(file_name, ios::out);
//        outfile_time << setprecision(10);
//        outfile_time << "m:," << out_m/out_counter << endl;
//        outfile_time << "daily_risk:," << out_daily_risk/out_counter << endl;
//        outfile_time << "trend:," << out_trend/out_counter << endl;
//        outfile_time << "real award:," << out_real_award/out_counter << endl << endl;
//        Portfolio* test_portfolio = new Portfolio[1];
//        test_portfolio[0].init(myData[0].size(), 250);
//        for(int j = 0; j < 250; j++){
//            test_portfolio[0].total_money[j] = out_daily_award[j]/out_counter;
//            cout << j  << ": " << out_daily_award[j]/out_counter << endl;
//            outfile_time << j << ":," << out_daily_award[j]/out_counter << endl;
//        }
//        countTrend(test_portfolio, 1, 10000000);
//        outfile_time << endl;
//        outfile_time << "m:," << test_portfolio[0].m << endl;
//        outfile_time << "daily_risk:," << test_portfolio[0].daily_risk << endl;
//        outfile_time << "trend:," << test_portfolio[0].trend << endl;
//        outfile_time << "a:," << test_portfolio[0].a << endl;
//        outfile_time << "b:," << test_portfolio[0].b << endl;
//    }

    return 0;
}
