#ifndef LNETWORK_H
#define LNETWORK_H
#include <random>
#include <cmath>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#define SCOPE 8
#define LAYERS 3
using namespace std;

int topScore=0;
int antiLottery=0;
double baseBiases[LAYERS][2*SCOPE]={0};
double baseWeights[LAYERS][2*SCOPE][2*SCOPE]={0};
double prec=100;
class lnetwork{

    private:
        double input[(SCOPE)];
        double output[4];
        double biases[LAYERS][2*SCOPE];
        double weights[LAYERS][2*SCOPE][2*SCOPE]; //Destination layer (in calcspace) ,Destination Id (in calcspace) ,Source ID (in calcspace or input)
        double calcSpace[LAYERS][2*(SCOPE)];
        int score;
        double neuronFunction(double x);
        void mutateBiases();
        void mutateWeights();
        void calculateOutput();
    public:
        lnetwork();
        ~lnetwork();
        int decision(double data[]);
        void reward(int number);

};

double lnetwork::neuronFunction(double x){
   /* double one=(double)(1);
    return (one/(exp(-x)+one)-0.5); */
    return x;

}

void lnetwork::mutateBiases(){
    for(int i=0; i<LAYERS-1; i++){
        for(int j=0; j<2*(SCOPE); j++){
            biases[i][j]+=(rand()%201-100)/prec;
        }
    }
}

void lnetwork::mutateWeights(){
    for(int i=0; i<LAYERS; i++){
        for(int j=0; j<2*(SCOPE); j++){
            for(int y=0; y<2*(SCOPE); y++){
              weights[i][j][y]+=(rand()%201-100)/prec;
            }
        }
    }
}

void lnetwork::calculateOutput(){
    int i;
    //prepare calc space
    for(i=0; i<LAYERS; i++){
        for(int j=0; j<2*(SCOPE); j++){
            calcSpace[i][j]=biases[i][j];
        }
    }
    //load input layer into calc space
    i=0;
    for(int j=0; j<2*(SCOPE); j++){
        for(int y=0; y<(SCOPE); y++){
            calcSpace[i][j]+=weights[i][j][y]*input[y];
        }
        calcSpace[i][j]=neuronFunction(calcSpace[i][j]);
    }
    //calculate remaining calc space layers
    for(i=1; i<LAYERS; i++){
        for(int j=0; j<2*(SCOPE); j++){
            for(int y=0; y<2*(SCOPE); y++){
                calcSpace[i][j]+=weights[i][j][y]*calcSpace[i-1][y];
            }
            calcSpace[i][j]=neuronFunction(calcSpace[i][j]);
        }
    }
    //fill output
    i=LAYERS-1;
    for(int j=0; j<4; j++){
        output[j]=calcSpace[i][j];
    }
}

lnetwork::lnetwork(){
    for(int i = 0; i < LAYERS; i++){
        for(int j = 0; j < 2*(SCOPE); j++){
            biases[i][j]=baseBiases[i][j];
            calcSpace[i][j]=0;
        }
    }
    for(int i = 0; i < LAYERS; i++){
        for(int j = 0; j < 2*(SCOPE); j++){
            for(int y = 0; y < 2*(SCOPE); y++){
                weights[i][j][y]=baseWeights[i][j][y];
            }
        }
    }


}


lnetwork::~lnetwork(){
}


void lnetwork::reward(int number){
  score+=number;
}


int lnetwork::decision(double data[]){
    int ret;
    for(int i=0; i<(SCOPE); i++){
        input[i]=data[i];
    }
    calculateOutput();
    if(output[0]>output[1]&&output[0]>output[2]){
        ret=0;
    }
    else if(output[1]>output[0]&&output[1]>output[2]){
        ret=1;
    }
    else{
        ret=2;
    }

    if(output[3]>=0){
        ret+=3;
    }
    return ret;
}



void loadNetwork(){
    ifstream load("nnsave.txt");
    string inpt;
    getline(load,inpt);
    stringstream str(inpt);
    str >> topScore;
    for(int i = 0; i < LAYERS-1; i++){
        for(int j = 0; j < 2*(SCOPE); j++){
            getline(load,inpt);
            stringstream str(inpt);
            str>>baseBiases[i][j];
        }
    }
    for(int i = 0; i < LAYERS; i++){
        for(int j = 0; j < 2*(SCOPE); j++){
            for(int y = 0; y < 2*(SCOPE); y++){
                getline(load,inpt);
                stringstream str(inpt);
                str>>baseWeights[i][j][y];
            }
        }
    }


}


#endif // LNETWORK_H
