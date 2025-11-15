/*
 * A simple program that computes the Fibonacci sequence using
 * a loop. The program does not do bounds checking...
 *
 */
#include <iostream>
#include <string>

using namespace std;

int main(){
    unsigned int n1 = 0;
    unsigned int n2 = 1;
    
    unsigned int stopValue = 0;
    string prompt1 = "Please enter the Fibonacci term you would like computed: ";
    string prompt2 = "Term ";
    string prompt3 = " in the Fibonacci sequence is: ";
    cout << prompt1;
    cin >> stopValue;
    
    unsigned int finalTerm = 0;
    if(stopValue == 1){
        finalTerm = n1;
    }
    else if(stopValue == 2){
        finalTerm = n2;
    }
    else{
        unsigned int currentTerm = 2;
        unsigned int temp = 0;
        while(currentTerm < stopValue){
            temp = n2;
            n2 = n1 + n2;
            n1 = temp;
            currentTerm += 1;
        }
        finalTerm = n2;
    }
    cout << prompt2;
    cout << stopValue;
    cout << prompt3;
    cout << finalTerm;
    cout << endl;
    return 0;
}
