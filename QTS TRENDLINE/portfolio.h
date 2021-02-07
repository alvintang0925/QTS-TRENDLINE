
#include <string>
#include <Eigen/Dense>
using namespace Eigen;
using namespace std;

class Stock {
public:
    string company_name;
    int idx;
    double* price_list;
    void init(int);
};

void Stock::init(int size) {
    price_list = new double[size];
}

class Portfolio {
public:
    int gen;
    int exp;
    int answer_counter = 0;
    int* data;
    int* investment_number;
    double* total_money;
    double* remain_fund;
    double m = 0;
    double a = -1;
    double b = -1;
    double daily_risk = 0;
    double trend = 0;
    Stock* constituent_stocks;
    double remain_money = 0;
    int stock_number = 0;
    int size = 0;
    int day_number = 0;
    double funds = 0;
    

    void init(int, int, int);
    int getDMoney();
    double getRemainMoney();
    double getNormalY(int);
    void countQuadraticYLine();
    double getQuadraticY(int);
    void copyP(Portfolio&);
    Portfolio();
    Portfolio(int, int, int);
    ~Portfolio();
};

Portfolio::Portfolio(){};

Portfolio::Portfolio(int size, int day_number, int funds) {
    this -> m = 0;
    this -> daily_risk = 0;
    this -> trend = 0;
    this -> stock_number = 0;
    this -> remain_money = 0;
    this -> funds = funds;
    this -> size = size;
    this -> day_number = day_number;
    data = new int[size];
    for (int j = 0; j < size; j++) {
        data[j] = 0;
    }
    investment_number = new int[size];
    constituent_stocks = new Stock[size];
    remain_fund = new double[size];
    total_money = new double[day_number];
}

Portfolio::~Portfolio() {
//    delete[] data;
//    delete[] investment_number;
//    delete[] constituent_stocks;
//    delete[] remain_fund;
//    delete[] total_money;
}

void Portfolio::init(int size, int day_number, int funds) {
    this -> funds = funds;
    this -> size = size;
    this -> day_number = day_number;
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

void Portfolio::countQuadraticYLine() {

    MatrixXd A(this -> day_number, 2);
    VectorXd Y(this -> day_number, 1);
    for (int j = 0; j < this -> day_number; j++) {
        for (int k = 0; k < 2; k++) {
            A(j, k) = pow(j + 1, 2 - k);
        }
        Y(j, 0) = this -> total_money[j] - this -> funds;
    }
    Vector2d theta = A.colPivHouseholderQr().solve(Y);
    this -> a = theta(0, 0);
    this -> b = theta(1, 0);
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
    
    this->gen = a.gen;
    this->exp = a.exp;
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
