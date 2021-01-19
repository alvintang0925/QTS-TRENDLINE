//
//  main.cpp
//  QTS TRENDLINE
//
//  Created by 唐健恆 on 2020/11/24.
//  Copyright © 2020 Alvin. All rights reserved.
//

#include <iostream>
#include <fstream>
#include <sstream>
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
#define FUNDS 10000000
#define PORTFOLIONUMBER 10
#define DELTA 0.0004
#define RUNTIMES 10000
#define EXPERIMENTNUMBER 50
#define QTSTYPE 2 //QTS 0, GQTS 1, GNQTS 2
#define TRENDLINETYPE 0 //linear 0, quadratic 1
#define SLIDETYPE 0 //M2M 0, Q2M 1, Q2Q 2, H2M 3, H2Q 4, H2H 5, Y2M 6, Y2Q 7, Y2H 8, Y2Y 9, M# 10, Q# 11, H# 12
string MODE = "train";
string STARTYEAR = "2019";
string STARTMONTH = "6";
string ENDYEAR = "2019";
string ENDMONTH = "6";
string fileDir = "myOutput3";


double* beta;
vector < vector<string> > myData;

class Stock{
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


void Stock::init(int size){
    price_list = new double[size];
}

class Portfolio{
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
    
    void init(int, int);
    int getDMoney();
    double getRemainMoney();
    double getNormalY(int);
    double getQuadraticY(int);
    void copyP(Portfolio&);
    Portfolio();
    ~Portfolio();
};

Portfolio::Portfolio(){
    
}

Portfolio::~Portfolio(){
    delete[] data;
    delete[] investment_number;
    delete[] constituent_stocks;
    delete[] remain_fund;
    delete[] total_money;
}

void Portfolio::init(int size, int day_number){
    this -> size = size;
    this -> day_number = day_number;
    data = new int[size];
    for(int j = 0; j < size; j++){
        data[j] = 0;
    }
    investment_number = new int[size];
    constituent_stocks = new Stock[size];
    remain_fund = new double[size];
    total_money = new double[day_number];
}

int Portfolio::getDMoney(){
    if(this -> stock_number != 0){
        return FUNDS / this -> stock_number;
    }else{
        return FUNDS;
    }
}

double Portfolio::getRemainMoney(){
    if(this -> stock_number != 0){
        double sum = 0;
        for(int j = 0; j < stock_number; j++){
            sum += (double)this->getDMoney() - (this -> investment_number[j] * this -> constituent_stocks[j].price_list[0]);
        }
        return (FUNDS % this -> stock_number) + sum;
    }else{
        return FUNDS;
    }
}

double Portfolio::getNormalY(int day){
    return this -> m * day + FUNDS;
}

double Portfolio::getQuadraticY(int day){
    return this -> a * pow(day, 2) + this -> b * day + FUNDS;
}

void Portfolio::copyP(Portfolio &a){
    delete[] (this -> data);
    delete[] (this -> investment_number);
    delete[] (this -> total_money);
    delete[] (this -> remain_fund);
    delete[] (this -> constituent_stocks);
    this -> data = new int[a.size];
    this -> investment_number = new int[a.size];
    this -> total_money = new double[a.day_number];
    this -> remain_fund = new double[a.size];
    this -> constituent_stocks = new Stock[a.size];
    for(int j = 0; j < a.size; j++){
        this -> data[j] = a.data[j];
    }
    for(int j = 0; j < a.stock_number; j++){
        this -> investment_number[j] = a.investment_number[j];
        this -> constituent_stocks[j] = a.constituent_stocks[j];
        this -> remain_fund[j] = a.remain_fund[j];
    }
    for(int j = 0; j < a.day_number; j++){
        this -> total_money[j] = a.total_money[j];
    }
    this -> m = a.m;
    this -> a = a.a;
    this -> b = a.b;
    this -> daily_risk = a.daily_risk;
    this -> trend = a.trend;
    this -> remain_money = a.remain_money;
    this -> stock_number = a.stock_number;
    this -> size = a.size;
    this -> day_number = a.day_number;
}

class Date{
public:
    tm date = {0};
    int slide_numer;
    string getYear();
    string getQ();
    string getMon();
    Date getRangeEnd(int);
    void slide();
};



string Date::getYear(){
    return to_string((this -> date.tm_year) + 1900);
}

string Date::getQ(){
    return "Q" + to_string((this -> date.tm_mon / 3)+1);
}

string Date::getMon(){
    if(this -> date.tm_mon > 8){
        return to_string((this -> date.tm_mon)+1);
    }else{
        return "0" + to_string((this -> date.tm_mon)+1);
    }
}

Date Date::getRangeEnd(int range){
    tm temp = this -> date;
    temp.tm_mon += range;
    time_t temp2 = mktime(&temp);
    tm* temp3 = localtime(&temp2);
    Date temp4;
    temp4.date = *temp3;
    return temp4;
}

void Date::slide(){
    tm temp = this -> date;
    temp.tm_mon += slide_numer;
    time_t temp2 = mktime(&temp);
    tm* temp3 = localtime(&temp2);
    this -> date = *temp3;

}

void countQuadraticYLine(Portfolio &portfolio){
    MatrixXd A(myData.size()-1, 2);
    VectorXd Y(myData.size()-1, 1);
    for(int j = 0; j < myData.size()-1; j++){
        for(int k = 0; k < 2; k++){
            A(j,k) = pow(j+1, 2-k);
        }
        Y(j,0) = portfolio.total_money[j] - FUNDS;
    }
    Vector2d theta = A.colPivHouseholderQr().solve(Y);
    portfolio.a = theta(0,0);
    portfolio.b = theta(1,0);
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


vector< vector<string> > readData(string filename){
    cout << filename << endl;
    ifstream inFile(filename, ios::in);
    string line;
    vector< vector<string> > data;
    
    if(!inFile){
        cout << "Open file failed!" <<endl;
        exit(1);
    }
    while(getline(inFile, line)){
        istringstream delime(line);
        string s;
        vector<string> line_data;
        while(getline(delime, s, ',')){
            if(s != "\r"){
                line_data.push_back(s);
            }
        }
        data.push_back(line_data);
    }
    return data;
}

void createStock(Stock* stock_list){
    for(int j = 0; j < myData[0].size(); j++){
        stock_list[j].idx = j;
        stock_list[j].init(myData.size()-1);
        
        for(int k = 0; k < myData.size(); k++){
            if(k == 0){
                stock_list[j].company_name = myData[k][j];
            }else{
                stock_list[j].price_list[k-1] = atof(myData[k][j].c_str());
            }
        }
    }
}

void initial(){
    gBest.trend = 0;
    gWorst.trend = DBL_MAX;
    gBest.init(myData[0].size(), myData.size());
    beta = new double[myData[0].size()];
    for(int j = 0; j < myData[0].size(); j++){
        beta[j] = 0.5;
    }
}

void gen_portfolio(Portfolio* portfolio_list, Stock* stock_list){
    for(int j = 0; j < PORTFOLIONUMBER; j++){
        
        portfolio_list[j].init(myData[0].size(),myData.size()-1);
        for(int k = 0; k < myData[0].size(); k++){
            
            double r = (double)rand() / (double)RAND_MAX;
            if(r > beta[k]){
                portfolio_list[j].data[k] = 0;
            }else{
                portfolio_list[j].data[k] = 1;
                portfolio_list[j].constituent_stocks[portfolio_list[j].stock_number] = stock_list[k];
                portfolio_list[j].stock_number++;
            }
        }
    }
}

void capitalLevel(Portfolio* portfolio_list){
    for(int j = 0; j < PORTFOLIONUMBER; j++){
        
        for(int k = 0; k < portfolio_list[j].stock_number; k++){
            portfolio_list[j].investment_number[k] = portfolio_list[j].getDMoney() / portfolio_list[j].constituent_stocks[k].price_list[0];
            portfolio_list[j].remain_fund[k] = portfolio_list[j].getDMoney() - (portfolio_list[j].investment_number[k] * portfolio_list[j].constituent_stocks[k].price_list[0]);
        }
        portfolio_list[j].total_money[0] = FUNDS;
        
    }
    
    for(int j = 0; j < myData.size()-2; j++){
        for(int k = 0; k < PORTFOLIONUMBER; k++){
            portfolio_list[k].total_money[j+1] = portfolio_list[k].getRemainMoney();
            for(int h = 0; h < portfolio_list[k].stock_number; h++){
                portfolio_list[k].total_money[j+1] += portfolio_list[k].investment_number[h] * portfolio_list[k].constituent_stocks[h].price_list[j+1];
            }
        }
    }
}

void countTrend(Portfolio* portfolio_list){
    for(int j = 0; j < PORTFOLIONUMBER; j++){
        double sum = 0;
        if(TRENDLINETYPE == 0){
            double x = 0;
            double y = 1;
            for(int k = 0; k < myData.size() - 2; k++){
                x += (k+2) * (portfolio_list[j].total_money[k+1] - FUNDS);
                y += (k+2) * (k+2);
            }
            if(portfolio_list[j].stock_number != 0){
                portfolio_list[j].m = x / y;
            }
            for (int k = 0; k < myData.size() - 1; k++){
                double Y;
                Y = portfolio_list[j].getNormalY(k+1);
                sum += (portfolio_list[j].total_money[k] - Y) * (portfolio_list[j].total_money[k] - Y);
            }
        }else if(TRENDLINETYPE == 1){
            countQuadraticYLine(portfolio_list[j]);
            for (int k = 0; k < myData.size() - 1; k++){
                double Y;
                Y = portfolio_list[j].getQuadraticY(k+1);
                sum += (portfolio_list[j].total_money[k] - Y) * (portfolio_list[j].total_money[k] - Y);
            }
            portfolio_list[j].m = (portfolio_list[j].getQuadraticY(myData.size() - 1) - portfolio_list[j].getQuadraticY(1)) / (myData.size()-2);
        }
        
        portfolio_list[j].daily_risk = sqrt(sum / (myData.size() - 1));
        
        if(portfolio_list[j].m < 0){
            portfolio_list[j].trend = portfolio_list[j].m * portfolio_list[j].daily_risk;
        }else{
            portfolio_list[j].trend = portfolio_list[j].m / portfolio_list[j].daily_risk;
        }
    }
}

void recordGAnswer(Portfolio* portfolio_list, int i){
    pBest.copyP(portfolio_list[0]);
    pWorst.copyP(portfolio_list[PORTFOLIONUMBER-1]);
    for(int j = 0; j < PORTFOLIONUMBER; j++){
        if(pBest.trend < portfolio_list[j].trend){
            pBest.copyP(portfolio_list[j]);
        }
        if(pWorst.trend > portfolio_list[j].trend){
            pWorst.copyP(portfolio_list[j]);
        }
    }
    
    if(gBest.trend < pBest.trend){
        gBest.copyP(pBest);
        g_gen = i + 1;
    }
    
    if(gWorst.trend > pWorst.trend){
        gWorst.copyP(pWorst);
    }
}

void adjBeta(){
    for(int j = 0; j < myData[0].size(); j++){
        if(QTSTYPE == 2){
            if(gBest.data[j] > pWorst.data[j]){
                if(beta[j] < 0.5){
                    beta[j] = 1 - beta[j];
                }
                beta[j] += DELTA;
            }else if(gBest.data[j] < pWorst.data[j]){
                if(beta[j] > 0.5){
                    beta[j] = 1 - beta[j];
                }
                beta[j] -= DELTA;
            }
        }else if(QTSTYPE ==1){
            if(gBest.data[j] > pWorst.data[j]){
                beta[j] += DELTA;
            }else if(gBest.data[j] < pWorst.data[j]){
                beta[j] -= DELTA;
            }
        }else{
            if(pBest.data[j] > pWorst.data[j]){
                beta[j] += DELTA;
            }else if(pBest.data[j] < pWorst.data[j]){
                beta[j] -= DELTA;
            }
        }
    }
}

void recordExpAnswer(int n){
    if(expBest.trend < gBest.trend){
        expBest.copyP(gBest);
        answer_gen = g_gen;
        answer_exp = n + 1;
        answer_counter = 1;
    }else if(expBest.trend == gBest.trend){
        answer_counter++;
    }
}

void outputFile(Date current_date){
    ofstream outfile;
    string file_name = fileDir + MODE + "_" + current_date.getYear() + "_" + current_date.getMon() + ".csv";
    outfile.open(file_name,ios::out);
    outfile << fixed << setprecision(10);
    
    
    outfile << "Exp times: ," << EXPERIMENTNUMBER << endl;
    outfile << "Iteration: ," << RUNTIMES << endl;
    outfile << "Element number: ," << PORTFOLIONUMBER << endl;
    outfile << "Delta: ," << DELTA << endl;
    outfile << "Init funds: ," << FUNDS << endl;
    outfile << "Final funds: ," << expBest.total_money[myData.size()-2] << endl;
    outfile << "Real award: ," <<  expBest.total_money[myData.size()-2] - FUNDS << endl << endl;
    outfile << "m: ," << expBest.m << endl;
    outfile << "Daily_risk: ," << expBest.daily_risk << endl;
    outfile << "Trend: ," << expBest.trend << endl << endl;
    
    if(TRENDLINETYPE == 0){
        countQuadraticYLine(expBest);
        double sum = 0;
        for (int k = 0; k < myData.size() - 1; k++){
            double Y;
            Y = expBest.getQuadraticY(k+1);
            sum += (expBest.total_money[k] - Y) * (expBest.total_money[k] - Y);
        }
        double c = (expBest.getQuadraticY(myData.size() - 1) - expBest.getQuadraticY(1)) / (myData.size()-2);
        double d = sqrt(sum / (myData.size() - 1));

        outfile << "Quadratic m:," << c << endl;
        outfile << "Quadratic daily risk:," << d << endl;
        outfile << "Quadratic trend:," << c/d << endl << endl;
    }else{
        outfile << "Quadratic trend line:," << expBest.a << "x^2 + " << expBest.b << "x + " << FUNDS << endl << endl;
        double x = 0;
        double y = 1;
        double sum = 0;
        for(int k = 0; k < myData.size() - 2; k++){
            x += (k+2) * (expBest.total_money[k+1] - FUNDS);
            y += (k+2) * (k+2);
        }
        
        double c = x / y;
        
        for (int k = 0; k < myData.size() - 1; k++){
            double Y;
            Y = expBest.getNormalY(k+1);
            sum += (expBest.total_money[k] - Y) * (expBest.total_money[k] - Y);
        }
        double d = sqrt(sum / (myData.size() - 1));
        
        outfile << "Linear m:," << c <<endl;
        outfile << "Linear daily risk:," << d << endl;
        outfile << "Linear trend:," << c/d << endl << endl;
    }
    
    outfile << "Best generation," << answer_gen << endl;
    outfile << "Best experiment," << answer_exp << endl;
    outfile << "Best answer times," << answer_counter << endl << endl;
    
    outfile << "Stock number: ," << expBest.stock_number << endl;
    outfile << "Stock#,";
    for(int j = 0; j < expBest.stock_number; j++){
        outfile << expBest.constituent_stocks[j].company_name << ",";
    }
    outfile << endl;
    
    outfile << "Number: ,";
    for(int j = 0; j < expBest.stock_number; j++){
        outfile << expBest.investment_number[j] << ",";
    }
    outfile << endl;
    
    outfile << "Distribue funds: ,";
    for(int j = 0; j < expBest.stock_number; j++){
        outfile << expBest.getDMoney() << ",";
    }
    outfile << endl;
    
    outfile << "Remain funds: ,";
    for(int j = 0; j < expBest.stock_number; j++){
        outfile << expBest.remain_fund[j] << ",";
    }
    outfile << endl;
    
    for(int j = 0; j < myData.size() - 1; j++){
        outfile << "FS(" << j+1 << "),";
        for(int k = 0; k < expBest.stock_number; k++){
            outfile << expBest.constituent_stocks[k].price_list[j] * expBest.investment_number[k] << ",";
        }
        outfile << expBest.total_money[j] << endl;
    }
    outfile << endl;
    
}

bool isFinish(tm current_data, tm finish_date){
    if(current_data.tm_year > finish_date.tm_year){
        return true;
    }
    if((current_data.tm_year == finish_date.tm_year) && (current_data.tm_mon > finish_date.tm_mon)){
        return true;
    }
    return false;
}
string getFilename(Date current_date){
    Date temp;
    switch(SLIDETYPE){
        case 0://M2M
            fileDir += "/M2M/";
            return "DJI_30/M2M/" + MODE + "_" + current_date.getYear() + "_" + current_date.getMon() + "(" + current_date.getYear() + " Q1).csv";
            break;
        case 1://Q2M
            fileDir += "/Q2M/";
            temp = current_date.getRangeEnd(2);
            if(current_date.getYear() != temp.getYear()){
                return "DJI_30/Q2M/" + MODE + "_" + current_date.getYear() + "_" + current_date.getMon() + "~" + temp.getYear() + "_" + temp.getMon() + "(" + current_date.getYear() + " Q1).csv";
            }else{
                return "DJI_30/Q2M/" + MODE + "_" + current_date.getYear() + "_" + current_date.getMon() + "-" + temp.getMon() + "(" + current_date.getYear() + " Q1).csv";
            }
            break;
        case 2://Q2Q
            fileDir += "/Q2Q/";
            return "DJI_30/Q2Q/" + MODE + "_" + current_date.getYear() + "_" + current_date.getQ() + "(" + current_date.getYear() + " Q1).csv";
            break;
        case 3://H2M
            fileDir += "/H2M/";
            temp = current_date.getRangeEnd(5);
            if(current_date.getYear() != temp.getYear()){
                return "DJI_30/H2M/" + MODE + "_" + current_date.getYear() + "_" + current_date.getMon() + "~" + temp.getYear() + "_" + temp.getMon() + "(" + current_date.getYear() + " Q1).csv";
            }else{
                return "DJI_30/H2M/" + MODE + "_" + current_date.getYear() + "_" + current_date.getMon() + "-" + temp.getMon() + "(" + current_date.getYear() + " Q1).csv";
            }
            break;
        case 4://H2Q
            fileDir += "/H2Q/";
            temp = current_date.getRangeEnd(5);
            if(current_date.getYear() != temp.getYear()){
                return "DJI_30/H2Q/" + MODE + "_" + current_date.getYear() + "_" + current_date.getQ() + "~" + temp.getYear() + "_" + temp.getQ() + "(" + current_date.getYear() + " Q1).csv";
            }else{
                return "DJI_30/H2Q/" + MODE + "_" + current_date.getYear() + "_" + current_date.getQ() + "-" + temp.getQ() + "(" + current_date.getYear() + " Q1).csv";
            }
            break;
        case 5://H2H
            fileDir += "/H2H/";
            temp = current_date.getRangeEnd(5);
            return "DJI_30/H2H/" + MODE + "_" + current_date.getYear() + "_" + current_date.getQ() + "-" + temp.getQ() + "(" + current_date.getYear() + " Q1).csv";
            break;
        case 6://Y2M
            fileDir += "/Y2M/";
            temp = current_date.getRangeEnd(11);
            if(current_date.getYear() != temp.getYear()){
                return "DJI_30/Y2M/" + MODE + "_" + current_date.getYear() + "_" + current_date.getMon() + "~" + temp.getYear() + "_" + temp.getMon() + "(" + current_date.getYear() + " Q1).csv";
            }else{
                return "DJI_30/Y2M/" + MODE + "_" + current_date.getYear() + "(" + current_date.getYear() + " Q1).csv";
            }
            break;
        case 7://Y2Q
            fileDir += "/Y2Q/";
            temp = current_date.getRangeEnd(11);
            if(current_date.getYear() != temp.getYear()){
                return "DJI_30/Y2Q/" + MODE + "_" + current_date.getYear() + "_" + current_date.getQ() + "~" + temp.getYear() + "_" + temp.getQ() + "(" + current_date.getYear() + " Q1).csv";
            }else{
                return "DJI_30/Y2Q/" + MODE + "_" + current_date.getYear() + "(" + current_date.getYear() + " Q1).csv";
            }
            break;
        case 8://Y2H
            fileDir += "/Y2H/";
            temp = current_date.getRangeEnd(11);
            if(current_date.getYear() != temp.getYear()){
                return "DJI_30/Y2H/" + MODE + "_" + current_date.getYear() + "_" + current_date.getQ() + "~" + temp.getYear() + "_" + temp.getQ() + "(" + current_date.getYear() + " Q1).csv";
            }else{
                return "DJI_30/Y2H/" + MODE + "_" + current_date.getYear() + "(" + current_date.getYear() + " Q1).csv";
            }
            break;
        case 9://Y2Y
            fileDir += "/Y2Y/";
            return "DJI_30/Y2Y/" + MODE + "_" + current_date.getYear() + "(" + current_date.getYear() + " Q1).csv";
            break;
        case 10://M#
            fileDir += "/M#/";
            return "DJI_30/M#/" + MODE + "_" + to_string(atoi(current_date.getYear().c_str())-1) + "_" + current_date.getMon() + "(" + to_string(atoi(current_date.getYear().c_str())-1) + " Q1).csv";
            break;
        case 11://Q#
            fileDir += "/Q#/";
            return "DJI_30/Q#/" + MODE + "_" + to_string(atoi(current_date.getYear().c_str())-1) + "_" + current_date.getQ() + "(" + to_string(atoi(current_date.getYear().c_str())-1) + " Q1).csv";
            break;
        case 12://H#
            fileDir += "/H#/";
            temp = current_date.getRangeEnd(5);
            return "DJI_30/H#/" + MODE + "_" + to_string(atoi(current_date.getYear().c_str())-1) + "_" + current_date.getQ() + "-" + temp.getQ() + "(" + to_string(atoi(current_date.getYear().c_str())-1) + " Q1).csv";
            break;
    }
}

void setDate(Date &current_date, Date &finish_date){
    current_date.date.tm_year = atoi(STARTYEAR.c_str()) - 1900;
    current_date.date.tm_mon = atoi(STARTMONTH.c_str()) - 1;
    current_date.date.tm_mday = 1;
    finish_date.date.tm_year = atoi(ENDYEAR.c_str()) - 1900;
    finish_date.date.tm_mon = atoi(ENDMONTH.c_str()) - 1;
    finish_date.date.tm_mday = 1;
    switch(SLIDETYPE){
        case 0:
        case 1:
        case 3:
        case 6:
        case 10:
            current_date.slide_numer = 1;
            break;
        case 2:
        case 4:
        case 7:
        case 11:
            current_date.slide_numer = 3;
            break;
        case 5:
        case 8:
        case 12:
            current_date.slide_numer = 6;
            break;
        case 9:
            current_date.slide_numer = 12;
            break;
    }
}

int main(int argc, const char * argv[]) {
    double START,END;
    START = clock();
    
    srand( 114 );
    cout<<fixed<<setprecision(10);
    Date current_date, finish_date;
    setDate(current_date, finish_date);
    
    do{
        string filename;
        filename = getFilename(current_date);
        myData = readData(filename);
        Stock* stock_list = new Stock[myData[0].size()];
        createStock(stock_list);
        expBest.trend = 0;
        expBest.init(myData[0].size(), myData.size());
        for(int n = 0; n < EXPERIMENTNUMBER; n++){
            initial();
            for(int i = 0; i < RUNTIMES; i++){
                Portfolio* portfolio_list = new Portfolio[PORTFOLIONUMBER];
                gen_portfolio(portfolio_list, stock_list);
                capitalLevel(portfolio_list);
                countTrend(portfolio_list);
    //            countQuadraticYLine(portfolio_list);
                recordGAnswer(portfolio_list, i);
                adjBeta();
                delete[] portfolio_list;
            }
            recordExpAnswer(n);
            cout << "___" << n << "___" << endl;
        }
        outputFile(current_date);
        current_date.slide();
        delete[] stock_list;
        cout<< "exp: " << answer_exp << endl;
        cout << "gen: " << answer_gen << endl;
        cout << "m:" << expBest.m << endl;
        cout << "risk:" << expBest.daily_risk << endl;
        cout << "trend: " << expBest.trend << endl;
        
    }while(!isFinish(current_date.date, finish_date.date));
    END = clock();
    double finish_time = (END - START) / CLOCKS_PER_SEC;
    ofstream outfile_time;
    string file_name = fileDir + "time.txt";
    outfile_time.open(file_name, ios::out);
    outfile_time << "total time: " << finish_time << " sec" << endl;
    cout << "total_time: " << (END - START) / CLOCKS_PER_SEC <<endl;
    return 0;
}
