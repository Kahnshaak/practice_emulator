/*
 * A simple program that computes the Fibonacci sequence using
 * a loop. The program does not do bounds checking...
 * Reg names used for variable names...
 */
#include <iostream>
#include <string>

using namespace std;

int main(){
    unsigned int R1 = 0;//n1
    unsigned int R2 = 1;//n2
    
    unsigned int R3 = 0;//stopValue
    string prompt1 = "Please enter the Fibonacci term you would like computed: ";//string variable names become directive labels...
    string prompt2 = "Term ";
    string prompt3 = " in the Fibonacci sequence is: ";
    cout << prompt1;
    cin >> R3;
    
    unsigned int R4 = 0;//finalTerm
    if(R3 == 1){
        R4 = R1;
    }
    else if(R3 == 2){
        R4 = R2;
    }
    else{
        unsigned int R5 = 2;//currentTerm
        unsigned int R6 = 0;//temp
        while(R5 < R3){
            R6 = R2;
            R2 = R1 + R2;
            R1 = R6;
            R5 += 1;
        }
        R4 = R2;
    }
    cout << prompt2;
    cout << R3;
    cout << prompt3;
    cout << R4;
    cout << endl;
    return 0;
}
