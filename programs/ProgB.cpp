/*
 * A simple program that performs the signed modulus/remainder operation.
 * The user is prompted to provide a dividend and divisor (in that
 * order). The program then computes the remainder via a function
 * called mod and prints the result to the screen.
 */

#include <iostream>
#include <string>

using namespace std;

int mod(int dividend, int divisor){
    int quotient = dividend / divisor;
    int remainder = dividend - (quotient * divisor);
    return remainder;
}

int main(){
    string prompt1 = "Please enter an integer dividend: ";
    string prompt2 = "Please enter an integer divisor: ";
    string prompt3 = " divided by ";
    string prompt4 = " results in a remainder of: ";
    
    int dividend = 0;
    int divisor = 0;
    
    cout << prompt1;
    cin >> dividend;
    cout << prompt2;
    cin >> divisor;
    
    int remainder = mod(dividend, divisor);
    
    cout << dividend;
    cout << prompt3;
    cout << divisor;
    cout << prompt4;
    cout << remainder;
    cout << endl;
    
    return 0;
}
