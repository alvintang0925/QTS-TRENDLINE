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
#include "portfolio.h"
#include "date.h"
using namespace std;
using namespace __fs::filesystem;

#define FUNDS 10000000
#define PORTFOLIONUMBER 10
#define DELTA 0.0004
#define RUNTIMES 10000
#define EXPERIMENTNUMBER 50
#define QTSTYPE 2 //QTS 0, GQTS 1, GNQTS 2
#define TRENDLINETYPE 0 //linear 0, quadratic 1
int SLIDETYPE = 0 ;//M2M 0, Q2M 1, Q2Q 2, H2M 3, H2Q 4, H2H 5, Y2M 6, Y2Q 7, Y2H 8, Y2Y 9, M# 10, Q# 11, H# 12
string MODE = "train";
string STARTYEAR;
string STARTMONTH;
string ENDYEAR;
string ENDMONTH;
string fileDir = "myOutputX";
int count_curve[3] = { 0 };
double current_funds = FUNDS;


double* beta;

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

vector<string> readSpeData(string filename, string title) {
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
            if(s == title){
                sw = true;
            }
        }
        sw = false;
    }
    inFile.close();
    return line_data;
}

void createStock(Stock* stock_list, int size, int day_number, vector<vector<string>> myData) {
    for (int j = 0; j < size; j++) {
        stock_list[j].idx = j;
        stock_list[j].init(day_number);

        for (int k = 0; k < day_number+1; k++) {
            if (k == 0) {
                stock_list[j].company_name = myData[k][j];
            }
            else {
                stock_list[j].price_list[k - 1] = atof(myData[k][j].c_str());
            }
        }
    }
}

void initial(int size, int day_number) {
//    gBest.trend = 0;
//    gWorst.trend = DBL_MAX;
//    gBest.init(size, day_number);
    beta = new double[size];
    for (int j = 0; j < size; j++) {
        beta[j] = 0.5;
    }
}

void gen_portfolio(Portfolio* portfolio_list, Stock* stock_list, int portfolio_number, string mode, int n, int i) {
    for (int j = 0; j < portfolio_number; j++) {
        portfolio_list[j].exp = n + 1;
        portfolio_list[j].gen = i + 1;
        portfolio_list[j].stock_number = 0;
        if(mode == "train"){
            for (int k = 0; k < portfolio_list[j].size; k++) {
                double r = (double)rand() / (double)RAND_MAX;
                if (r > beta[k]) {
                    portfolio_list[j].data[k] = 0;
                }
                else {
                    portfolio_list[j].data[k] = 1;
                }
            }
        }
        
        for(int k = 0; k < portfolio_list[j].size; k++){
            if(portfolio_list[j].data[k] == 1){
                portfolio_list[j].constituent_stocks[portfolio_list[j].stock_number] = stock_list[k];
                portfolio_list[j].stock_number++;
            }
        }
    }
}

void gen_testPortfolio(Portfolio* portfolio_list, Stock* stock_list, int portfolio_number, string mode, vector<vector<string>> myData, vector<string>myTrainData) {
    for (int j = 0; j < portfolio_number; j++) {
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
        
        for (int k = 0; k < portfolio_list[j].day_number - 1; k++) {
            portfolio_list[j].total_money[k + 1] = portfolio_list[j].getRemainMoney();
            for (int h = 0; h < portfolio_list[j].stock_number; h++) {
                portfolio_list[j].total_money[k + 1] += portfolio_list[j].investment_number[h] * portfolio_list[j].constituent_stocks[h].price_list[k + 1];
            }
        }
    }
}

void countTrend(Portfolio* portfolio_list, int porfolio_number, double funds) {
    for (int j = 0; j < porfolio_number; j++) {
        double sum = 0;
        if (TRENDLINETYPE == 0) {
            //portfolio_list[j].countQuadraticYLine();
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
            portfolio_list[j].countQuadraticYLine();
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

void recordGAnswer(Portfolio* portfolio_list, Portfolio& gBest, Portfolio& gWorst, Portfolio& pBest, Portfolio& pWorst) {
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
//        if(gBest.a < 0 || pBest.a >= 0){
            gBest.copyP(pBest);
//        }
    }
    
    if (gWorst.trend > pWorst.trend) {
        gWorst.copyP(pWorst);
    }
}

void adjBeta(Portfolio& best, Portfolio& worst) {
    for (int j = 0; j < best.size; j++) {
        if (QTSTYPE == 2) {
            if (best.data[j] > worst.data[j]) {
                if (beta[j] < 0.5) {
                    beta[j] = 1 - beta[j];
                }
                beta[j] += DELTA;
            }
            else if (best.data[j] < worst.data[j]) {
                if (beta[j] > 0.5) {
                    beta[j] = 1 - beta[j];
                }
                beta[j] -= DELTA;
            }
        }
        else if (QTSTYPE == 1) {
            if (best.data[j] > worst.data[j]) {
                beta[j] += DELTA;
            }
            else if (best.data[j] < worst.data[j]) {
                beta[j] -= DELTA;
            }
        }
        else {
            if (best.data[j] > worst.data[j]) {
                beta[j] += DELTA;
            }
            else if (best.data[j] < worst.data[j]) {
                beta[j] -= DELTA;
            }
        }
    }
}

void recordExpAnswer(Portfolio& expBest, Portfolio& gBest) {
    if (expBest.trend < gBest.trend) {
        expBest.copyP(gBest);
        expBest.answer_counter = 1;
    }
    else if (expBest.trend == gBest.trend) {
        expBest.answer_counter++;
    }
}

string getOutputFilename(Date current_date, string mode, string file_dir, string type){
    return file_dir + "/" + type + "/" + mode + "/" + mode + "_" + current_date.getYear() + "_" + current_date.getMon() + ".csv";
}

void outputFile(Date current_date, Portfolio& portfolio, string mode, string file_name) {
    ofstream outfile;
    outfile.open(file_name, ios::out);
    outfile << fixed << setprecision(10);


    outfile << "Exp times: ," << EXPERIMENTNUMBER << endl;
    outfile << "Iteration: ," << RUNTIMES << endl;
    outfile << "Element number: ," << PORTFOLIONUMBER << endl;
    outfile << "Delta: ," << DELTA << endl;
    outfile << "Init funds: ," << portfolio.funds << endl;
    outfile << "Final funds: ," << portfolio.total_money[portfolio.day_number - 1] << endl;
    outfile << "Real award: ," << portfolio.total_money[portfolio.day_number - 1] - portfolio.funds << endl << endl;
    outfile << "m: ," << portfolio.m << endl;
    outfile << "Daily_risk: ," << portfolio.daily_risk << endl;
    outfile << "Trend: ," << portfolio.trend << endl << endl;

    if (TRENDLINETYPE == 0) {
        portfolio.countQuadraticYLine();
        double sum = 0;
        for (int k = 0; k < portfolio.day_number; k++) {
            double Y;
            Y = portfolio.getQuadraticY(k + 1);
            sum += (portfolio.total_money[k] - Y) * (portfolio.total_money[k] - Y);
        }
        double c = (portfolio.getQuadraticY(portfolio.day_number) - portfolio.getQuadraticY(1)) / (portfolio.day_number - 1);
        double d = sqrt(sum / (portfolio.day_number));
        
        outfile << "Quadratic trend line:," << portfolio.a << "x^2 + " << portfolio.b << "x + " << FUNDS << endl << endl;
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
        for (int k = 0; k < portfolio.day_number - 1; k++) {
            x += (k + 2) * (portfolio.total_money[k + 1] - portfolio.funds);
            y += (k + 2) * (k + 2);
        }

        double c = x / y;
        for (int k = 0; k < portfolio.day_number; k++) {
            double Y;
            Y = c * (k + 1) + portfolio.funds;
            sum += (portfolio.total_money[k] - Y) * (portfolio.total_money[k] - Y);
        }
        double d = sqrt(sum / (portfolio.day_number));

        outfile << "Linear m:," << c << endl;
        outfile << "Linear daily risk:," << d << endl;
        if(c < 0){
            outfile << "Linear trend:," << c * d << endl << endl;
        }else{
            outfile << "Linear trend:," << c / d << endl << endl;
        }
    }

    outfile << "Best generation," << portfolio.gen << endl;
    outfile << "Best experiment," << portfolio.exp << endl;
    outfile << "Best answer times," << portfolio.answer_counter << endl << endl;

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

    for (int j = 0; j < portfolio.day_number; j++) {
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
string getPriceFilename(Date current_date, string mode, string TYPE, int train_range) {
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
    return "";
}


void setDate(Date& current_date, Date& finish_date, string& TYPE, int& train_range) {
    switch (SLIDETYPE) {
    case 0:
        STARTYEAR = "2009";
        STARTMONTH = "12";
        ENDYEAR = "2019";
        ENDMONTH = "11";
        TYPE = "M2M";
        train_range = 1;
        current_date.slide_numer = 1;
        break;
    case 1:
        STARTYEAR = "2009";
        STARTMONTH = "10";
        ENDYEAR = "2019";
        ENDMONTH = "9";
        TYPE = "Q2M";
        train_range = 3;
        current_date.slide_numer = 1;
        break;
    case 2:
        STARTYEAR = "2009";
        STARTMONTH = "10";
        ENDYEAR = "2019";
        ENDMONTH = "7";
        TYPE = "Q2Q";
        train_range = 3;
        current_date.slide_numer = 3;
        break;
    case 3:
        STARTYEAR = "2009";
        STARTMONTH = "7";
        ENDYEAR = "2019";
        ENDMONTH = "6";
        TYPE = "H2M";
        train_range = 6;
        current_date.slide_numer = 1;
        break;
    case 4:
        STARTYEAR = "2009";
        STARTMONTH = "7";
        ENDYEAR = "2019";
        ENDMONTH = "4";
        TYPE = "H2Q";
        train_range = 6;
        current_date.slide_numer = 3;
        break;
    case 5:
        STARTYEAR = "2009";
        STARTMONTH = "7";
        ENDYEAR = "2019";
        ENDMONTH = "1";
        TYPE = "H2H";
        train_range = 6;
        current_date.slide_numer = 6;
        break;
    case 6:
        STARTYEAR = "2009";
        STARTMONTH = "2";
        ENDYEAR = "2018";
        ENDMONTH = "12";
        TYPE = "Y2M";
        train_range = 12;
        current_date.slide_numer = 1;
        break;
    case 7:
        STARTYEAR = "2009";
        STARTMONTH = "1";
        ENDYEAR = "2018";
        ENDMONTH = "10";
        TYPE = "Y2Q";
        train_range = 12;
        current_date.slide_numer = 3;
        break;
    case 8:
        STARTYEAR = "2009";
        STARTMONTH = "7";
        ENDYEAR = "2018";
        ENDMONTH = "7";
        TYPE = "Y2H";
        train_range = 12;
        current_date.slide_numer = 6;
        break;
    case 9:
        STARTYEAR = "2009";
        STARTMONTH = "1";
        ENDYEAR = "2018";
        ENDMONTH = "1";
        TYPE = "Y2Y";
        train_range = 12;
        current_date.slide_numer = 12;
        break;
    case 10:
        STARTYEAR = "2010";
        STARTMONTH = "1";
        ENDYEAR = "2019";
        ENDMONTH = "12";
        TYPE = "M#";
        train_range = 1;
        current_date.slide_numer = 1;
        break;
    case 11:
        STARTYEAR = "2010";
        STARTMONTH = "1";
        ENDYEAR = "2019";
        ENDMONTH = "10";
        TYPE = "Q#";
        train_range = 3;
        current_date.slide_numer = 3;
        break;
    case 12:
        STARTYEAR = "2010";
        STARTMONTH = "1";
        ENDYEAR = "2019";
        ENDMONTH = "7";
        TYPE = "H#";
        train_range = 6;
        current_date.slide_numer = 6;
        break;
    }
    if(MODE == "train"){
        train_range = 0;
    }
    current_date.date.tm_year = atoi(STARTYEAR.c_str()) - 1900;
    current_date.date.tm_mon = atoi(STARTMONTH.c_str()) - 1;
    current_date.date.tm_mday = 1;
    finish_date.date.tm_year = atoi(ENDYEAR.c_str()) - 1900;
    finish_date.date.tm_mon = atoi(ENDMONTH.c_str()) - 1;
    finish_date.date.tm_mday = 1;
}

void createDir(string file_dir, string type){
    create_directory(file_dir);
    create_directory(file_dir + "/" + type);
    create_directory(file_dir + "/" + type + "/train");
    create_directory(file_dir + "/" + type + "/test");
}




int main(int argc, const char* argv[]) {
    
    for(int s = 0; s < 13; s++){
        SLIDETYPE = s;
    double START, END;
    START = clock();
    srand(114);
    cout << fixed << setprecision(10);
    Date current_date, finish_date;
    string TYPE;
    int train_range;
    setDate(current_date, finish_date, TYPE, train_range);
    createDir(fileDir, TYPE);
    
    do {
        if(MODE == "train"){
            vector<vector<string>> myData = readData(getPriceFilename(current_date, MODE, TYPE, train_range));
            Stock* stock_list = new Stock[myData[0].size()];
            createStock(stock_list, myData[0].size(), myData.size() - 1, myData);
            Portfolio expBest(myData[0].size(), myData.size() - 1, FUNDS);
            for (int n = 0; n < EXPERIMENTNUMBER; n++) {
                Portfolio* portfolio_list = new Portfolio[PORTFOLIONUMBER];
                Portfolio gBest(myData[0].size(), myData.size() - 1, FUNDS);
                Portfolio gWorst(myData[0].size(), myData.size() - 1, FUNDS);
                gWorst.trend = DBL_MAX;
                initial(myData[0].size(), myData.size() - 1);
                for (int i = 0; i < RUNTIMES; i++) {
                    Portfolio pBest(myData[0].size(), myData.size() - 1, FUNDS);
                    Portfolio pWorst(myData[0].size(), myData.size() - 1, FUNDS);
                    for(int j = 0; j < PORTFOLIONUMBER; j++){
                        portfolio_list[j] = Portfolio(myData[0].size(), myData.size() - 1, FUNDS);
                    }
                    gen_portfolio(portfolio_list, stock_list, PORTFOLIONUMBER, "train", n, i);
                    capitalLevel(portfolio_list, PORTFOLIONUMBER, FUNDS);
                    countTrend(portfolio_list, PORTFOLIONUMBER, FUNDS);
                    recordGAnswer(portfolio_list, gBest, gWorst, pBest, pWorst);
                    adjBeta(gBest, pWorst);
                }
                delete[] portfolio_list;
                recordExpAnswer(expBest, gBest);
                cout << "___" << n << "___" << endl;
            }
            outputFile(current_date, expBest, "train", getOutputFilename(current_date, MODE, fileDir, TYPE));
            delete[] stock_list;
            cout << "exp: " << expBest.exp << endl;
            cout << "gen: " << expBest.gen << endl;
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
            vector<string> myTrainData = readSpeData(getOutputFilename(current_date.getRangeEnd(-1 * train_range), "train", fileDir, TYPE), "stock#");
            vector < vector<string> > myData = readData(getPriceFilename(current_date, MODE, TYPE, train_range));
            Stock* test_stock_list = new Stock[myData[0].size()];
            createStock(test_stock_list, myData[0].size(), myData.size() - 1, myData);
            Portfolio* test_portfolio = new Portfolio[1];
            gen_testPortfolio(test_portfolio, test_stock_list, 1, "test", myData, myTrainData);
//            test_portfolio[0].funds = current_funds;
            
            capitalLevel(test_portfolio, 1, current_funds);
            countTrend(test_portfolio, 1, current_funds);
//            current_funds = test_portfolio[0].total_money[test_portfolio[0].day_number - 1];
            
            outputFile(current_date, test_portfolio[0], "test", getOutputFilename(current_date, MODE, fileDir, TYPE));
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
    }
    return 0;
}
