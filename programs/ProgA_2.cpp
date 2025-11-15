/*
 * A simple program that computes the Fibonacci sequence using
 * a loop. The program does not do bounds checking...
 * Reg names used for variable names...
 * Organize code by sections, insert control flow labels
 * Annotate with assembly like comments...
 */
#include <iostream>
#include <string>

using namespace std;

int main(){
    //Data Section
    //string variable names become directive labels...
    string prompt1 = "Please enter the Fibonacci term you would like computed: ";
    string prompt2 = "Term ";
    string prompt3 = " in the Fibonacci sequence is: ";
    
    //Code Section
    unsigned int R1 = 0;//n1
    unsigned int R2 = 1;//n2
    unsigned int R3 = 0;//stopValue
    cout << prompt1;
    cin >> R3;
    
    unsigned int R4 = 0;//finalTerm
ONE:
    if(R3 == 1){//BNE TWO
        R4 = R1;//after this jmp to PRINT
    }
TWO:
    else if(R3 == 2){//BNE ELSE
        R4 = R2;//after this jmp to PRINT
    }
ELSE:
    else{
        unsigned int R5 = 2;//currentTerm
        unsigned int R6 = 0;//temp
LOOP1:
        while(R5 < R3){//if this isn't true jmp to DONE1
CONT:
            R6 = R2;
            R2 = R1 + R2;
            R1 = R6;
            R5 += 1;//after this jmp to LOOP1
        }
DONE1:
        R4 = R2;
    }
PRINT:
    cout << prompt2;
    cout << R3;
    cout << prompt3;
    cout << R4;
    cout << endl;
    return 0;
}
