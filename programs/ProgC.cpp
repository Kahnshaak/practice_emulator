/*
 * A simple program that computes the Fibonacci sequence using
 * recursion. The program does not do bounds checking...
 *
 */
#include <iostream>
#include <string>

using namespace std;

int fib(unsigned int N){
    if(N == 1){
        return 0;
    }
    if(N == 2){
        return 1;
    }
    return fib(N - 1) + fib(N - 2);
}

int main(){
    unsigned int stopValue = 0;
    string prompt1 = "Please enter the Fibonacci term you would like computed: ";
    string prompt2 = "Term ";
    string prompt3 = " in the Fibonacci sequence is: ";
    cout << prompt1;
    cin >> stopValue;
    
    unsigned int result = fib(stopValue);
    
    cout << prompt2;
    cout << stopValue;
    cout << prompt3;
    cout << result;
    cout << endl;
    return 0;
}
