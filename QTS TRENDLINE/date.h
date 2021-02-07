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
    temp.tm_mon += this->slide_numer;
    time_t temp2 = mktime(&temp);
    tm* temp3 = localtime(&temp2);
    this->date = *temp3;
}
