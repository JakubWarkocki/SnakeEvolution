#ifndef LEVELGEN_H
#define LEVELGEN_H
#include <random>
#include <iostream>
#include <vector>
#define CHUNK_SIZE 16
#define CHUNK_BOARD_SIZE 64
int chunkBoard[CHUNK_BOARD_SIZE][CHUNK_BOARD_SIZE]={1};
const int splitChance=5;
const int turnChance=16;
int run=1;
class Digger{

    private:
        int X;
        int Y;
        int direction;
        int turn(int dir, int rl);
        void turnL();
        void turnR();

    public:
        int energy;
        void moveAndDig();
        Digger(int spawnX, int spawnY, int spawnDirection, int spawnEnergy);


};
vector<Digger> diggers;
Digger::Digger(int spawnX, int spawnY, int spawnDirection, int spawnEnergy){
    X=spawnX;
    Y=spawnY;
    direction=spawnDirection;
    energy=spawnEnergy;
}
int Digger::turn(int dir, int rl){
    if(dir==0&&rl==-1){
        return 3;
    }
    else{
        return (dir+rl)%4;
    }
}


void Digger::turnL(){
    direction=turn(direction,-1);
}
void Digger::turnR(){
    direction=turn(direction,1);
}
void Digger::moveAndDig(){
    if(energy==0){
        return;
    }
    if(X<=0||Y<=0||X>=CHUNK_BOARD_SIZE-1||Y>=CHUNK_BOARD_SIZE-1){
        energy=0;
        return;
    }
    else{
        if(energy){
        chunkBoard[X][Y]=0;
        energy--;
        }
    }
    if(energy){
    switch(direction){
        case 0:
            X++;
            break;
        case 1:
            Y++;
            break;
        case 2:
            X--;
            break;
        case 3:
            Y--;
            break;

    }
    }
    if(chunkBoard[X][Y]==0){
        energy=0;
    }

    if(energy){
    if(rand()%turnChance==0){
        if(rand()%2){
            turnL();
        }
        else{
            turnR();
        }
    }
    else if(rand()%splitChance==0){
        if(rand()%2){
            diggers.push_back(Digger(X,Y,turn(direction,-1),energy/2));
        }
        if(rand()%2){
            diggers.push_back(Digger(X,Y,turn(direction,1),energy/2));
        }
    }
    }
}

void generateLevel(int seed){
            srand(seed);
            for(int i=0; i<CHUNK_BOARD_SIZE; i++){
                for(int j=0; j<CHUNK_BOARD_SIZE; j++){
                    chunkBoard[i][j]=1;
                }
            }
            diggers.push_back(Digger(CHUNK_BOARD_SIZE/2,CHUNK_BOARD_SIZE/2,0,256));
            diggers.push_back(Digger(CHUNK_BOARD_SIZE/2,CHUNK_BOARD_SIZE/2,1,256));
            diggers.push_back(Digger(CHUNK_BOARD_SIZE/2,CHUNK_BOARD_SIZE/2,2,256));
            diggers.push_back(Digger(CHUNK_BOARD_SIZE/2,CHUNK_BOARD_SIZE/2,3,256));
            while(true){
                run=0;
                for (auto& digger : diggers) {
                    if(digger.energy){
                        run++;
                        digger.moveAndDig();
                    }
                }
                if(run==0){
                    break;
                }
            }
        }
#endif // LEVELGEN_H
