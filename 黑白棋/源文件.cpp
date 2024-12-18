﻿#include <iostream>
#include <cstdio>
#include <cstring>
#include <string>
#include <cstdlib>
#include <vector>
#include <queue>
#include <random>
#include <fstream>
#include <windows.h>

#define oo 1e8

using namespace std;

int realBoard[10][10]; //当前棋盘局面
int calcBoard[10][10]; //每个空点，当前执旗手可吃子数
bool currPlayer; //当前执棋，黑1白0
bool currOppo; //选择对手，机1人0
const int changeX[10] = { 0, 1, 0, -1, 0, 1, 1, -1, -1 };
const int changeY[10] = { 0, 0, 1, 0, -1, 1, -1, 1, -1 }; //用于查找、翻转时更改坐标
int diffLevel; //难度
vector<int> LPAVX, LPAVY;
int bestValue;
vector<int> LPANEX, LPANEY;
int biggestNum;
int canEat[10][10];
bool notReallyReverse = 0; //用于最低等AI，模拟棋子翻转
bool pveSide;
int searchDepth = 0; //搜索层数。如无例外，最后应停留在Max层，故应为偶数。
const int boardValue[10][10] = //棋盘权值估计
{
	{0},
	{0, 20,-10,10, 5, 5,10,-10, 20},
	{0,-10,-15, 1, 1, 1, 1,-15,-10},
	{0, 10,  1 ,3, 2, 2, 3,  1, 10},
	{0, 10,  1 ,3, 2, 2, 3,  1, 10},	
	{0, 10,  1 ,3, 2, 2, 3,  1, 10},
	{0, 10,  1 ,3, 2, 2, 3,  1, 10},
	{0,-10,-15, 1, 1, 1, 1,-15,-10},
	{0, 20,-10,10, 5, 5,10,-10, 20}
};
int pieceCnt[5];
struct chessData {
	bool player;
	int x, y;
	int cnt;
	int rx[80], ry[80];
} currChess[80];
bool repentance = 1; //悔棋设置
bool suggestionMood = 1; //提示设置
int A1 = 30, A2 = 50, A3 = 20; //估值函数权重
bool fileStatus = 0; //文档状态

string boolToStr(bool b)
{
	return b ? "ON" : "OFF";
}

string numToColor(int num)
{
	return num ? "○" : "●"; 
}

void setColor(int color)
{
	WORD wColor = ((0 & 0x0F) << 4) + (color & 0x0F);
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), wColor);
}

void printBoard()
{
	system("cls");
	cout << " ○ Count: " << pieceCnt[1] << endl;
	cout << " ● Count: " << pieceCnt[0] << endl << endl;
	cout << "                  ";
	for (int j = 1; j <= 8; ++j)
		cout << " " << j << " ";
	cout << endl << endl;
	for (int i = 1; i <= 8; ++i)
	{
		cout << "                " << i << "  ";
		for (int j = 1; j <= 8; ++j)
		{
			if (calcBoard[i][j])
				cout << "**";
			else
			{
				if (i == currChess[pieceCnt[0] + pieceCnt[1]].x && j == currChess[pieceCnt[0] + pieceCnt[1]].y)
					setColor(6);
				if (realBoard[i][j] == -1)
					cout << "__";
				else if (realBoard[i][j])
					cout << "○";
				else
					cout << "●";
				if (i == currChess[pieceCnt[0] + pieceCnt[1]].x && j == currChess[pieceCnt[0] + pieceCnt[1]].y)
					setColor(7);
			}
			cout << " ";
		}
		cout << endl << endl;
	}
	cout << endl;
	return;
}

void leavePiece(int x, int y)
{
	realBoard[x][y] = currPlayer;
	pieceCnt[currPlayer]++;
	currChess[pieceCnt[0] + pieceCnt[1]].x = x;
	currChess[pieceCnt[0] + pieceCnt[1]].y = y;
	currChess[pieceCnt[0] + pieceCnt[1]].player = currPlayer;
	return;
}

void findForCalc(int x, int y, int dir)
{
	x += changeX[dir], y += changeY[dir];
	if (!x || !y || x > 8 || y > 8)
		return;
	if (realBoard[x][y] == -1)
	{
		calcBoard[x][y]++;
		return;
	}
	if (realBoard[x][y] != currPlayer)
		findForCalc(x, y, dir);
	return;
}

void calc()
{
	memset(calcBoard, 0, sizeof(calcBoard));
	for (int i = 1; i <= 8; ++i)
		for (int j = 1; j <= 8; ++j)
		{
			if (realBoard[i][j] == currPlayer)
			{
				for (int k = 1; k <= 8; ++k)
				{
					int x = i + changeX[k], y = j + changeY[k];
					if (!x || !y || x > 8 || y > 8)
						continue;
					if (realBoard[x][y] == !currPlayer)
						findForCalc(x, y, k);
				}
			}
		}
	return;
}

void repent()
{
	int step = pieceCnt[0] + pieceCnt[1];
	currPlayer = currChess[step].player;
	realBoard[currChess[step].x][currChess[step].y] = -1; //撤销下棋
	pieceCnt[currPlayer]--;
	int size = currChess[step].cnt;
	for (int i = 0; i < size; ++i) //撤销翻转
	{
		realBoard[currChess[step].rx[currChess[step].cnt]][currChess[step].ry[currChess[step].cnt]] = !currPlayer;
		currChess[step].cnt--;
		pieceCnt[currPlayer]--;
		pieceCnt[!currPlayer]++;
	}
	calc();
	return;
}

int canEatCnt = 0;
bool findForReverse(int x, int y, int dir)
{
	canEatCnt = 0;
	x += changeX[dir], y += changeY[dir];
	if (!x || !y || x > 8 || y > 8)
		return 0;
	if (realBoard[x][y] == !currPlayer)
	{
		if (findForReverse(x, y, dir))
		{
			if (!notReallyReverse)
			{
				int steps = pieceCnt[0] + pieceCnt[1];
				realBoard[x][y] = currPlayer;
				currChess[steps].rx[++currChess[steps].cnt] = x;
				currChess[steps].ry[currChess[steps].cnt] = y;
				pieceCnt[currPlayer]++;
				pieceCnt[!currPlayer]--;
			}
			else
				canEatCnt++;
			return 1;
		}
		else
			return 0;
	}
	if (realBoard[x][y] == currPlayer)
		return 1;
	return 0;
}
void Reverse(int x, int y)
{
	if (!notReallyReverse)
		currChess[pieceCnt[0] + pieceCnt[1]].cnt = 0;
	for (int k = 1; k <= 8; ++k)
	{
		int newX = x + changeX[k], newY = y + changeY[k];
		if (!newX || !newY || newX > 8 || newY > 8)
			continue;
		if (realBoard[newX][newY] == !currPlayer)
		{
			findForReverse(x, y, k);
			canEat[x][y] += canEatCnt;
		}
	}
	return;
}

bool allZero()
{
	for (int i = 1; i <= 8; ++i)
		for (int j = 1; j <= 8; ++j)
			if (calcBoard[i][j])
				return 0;
	return 1;
}

void LeavePieceAccNumEaten()
{
	//cout << "flag1" << endl;
	biggestNum = 0;
	LPANEX.clear();
	LPANEY.clear();
	memset(canEat, 0, sizeof(canEat));
	notReallyReverse = 1;
	for (int i = 1; i <= 8; ++i)
		for (int j = 1; j <= 8; ++j)
			if (calcBoard[i][j])
			{
				//cout << "flag2" << endl;
				Reverse(i, j);
				if (canEat[i][j] > biggestNum)
				{
					LPANEX.clear();
					LPANEY.clear();
				}
				if (canEat[i][j] >= biggestNum)
				{
					biggestNum = canEat[i][j];
					LPANEX.push_back(i);
					LPANEY.push_back(j);
				}
			}
	notReallyReverse = 0;
	random_device rd;
	mt19937 gen(rd());
	uniform_int_distribution<int> dis(0, LPANEX.size() - 1);
	int randomNumber = dis(gen);
	int x = LPANEX[randomNumber], y = LPANEY[randomNumber];
	leavePiece(x, y);
	Reverse(x, y);
	return;
}

void LeavePieceAccValueEasy()
{
	bestValue = -oo;
	LPAVX.clear();
	LPAVY.clear();
	for (int i = 1; i <= 8; ++i)
		for (int j = 1; j <= 8; ++j)
			if (calcBoard[i][j])
			{
				if (boardValue[i][j] > bestValue)
				{
					LPAVX.clear();
					LPAVY.clear();
				}
				if (boardValue[i][j] >= bestValue)
				{
					bestValue = boardValue[i][j];
					LPAVX.push_back(i);
					LPAVY.push_back(j);
				}
			}
	random_device rd;
	mt19937 gen(rd());
	uniform_int_distribution<int> dis(0, LPAVX.size()-1);
	int randomNumber = dis(gen);
	int x = LPAVX[randomNumber], y = LPAVY[randomNumber];
	leavePiece(x, y);
	Reverse(x, y);
	return;
}

bool playerBfSim;
bool stable[10][10][5];
const int cx[5] = { 0, 1, 0, -1 };
const int cy[5] = { 1, 0, -1, 0 };
const int corner[5][2] =
{
	{1, 1},
	{1, 8},
	{8, 8},
	{8, 1}
};
void findStable(bool player)
{
	for (int i = 0; i < 4; ++i)
	{
		int x = corner[i][0],
			y = corner[i][1];
		if (realBoard[x][y] == player)
			for (int j = 0; j <= 1; ++j)
			{
				do {
					stable[x][y][player] = 1;
					x += cx[(i + j) % 4];
					y += cy[(i + j) % 4];
				} while (x > 0 && x < 9 && y > 0 && y < 9 && realBoard[x][y] == player);
			}
	}
	return;
}
int evaluateCurrBoard()
{
	calc();
	int M = 0;
	for (int i = 1; i <= 8; ++i)
		for (int j = 1; j <= 8; ++j)
			if (calcBoard[i][j])
				M++;
	memset(stable, 0, sizeof(stable));
	findStable(currPlayer);
	findStable(!currPlayer);
	int S1 = 0, S2 = 0;
	for (int i = 1; i <= 8; ++i)
		for (int j = 1; j <= 8; ++j)
		{
			if (stable[i][j][currPlayer])
				S1++;
			if (stable[i][j][!currPlayer])
				S2++;
		}
	int totW = 0;
	for (int i = 1; i <= 8; ++i)
		for (int j = 1; j <= 8; ++j)
		{
			if (realBoard[i][j] == currPlayer)
				totW += boardValue[i][j];
			if (realBoard[i][j] == !currPlayer)
				totW -= boardValue[i][j];
		}
	return  A1 * M + A2 * (S1 - S2) + A3 * totW;
}
int LeavePieceSimulate(int x, int y, int depth)
{
	if (depth == searchDepth)
		return evaluateCurrBoard();
	
	//模拟放棋
	leavePiece(x, y);
	Reverse(x, y);
	currPlayer = !currPlayer;
	calc();

	//特例
	if (allZero()) //没得可下
	{
		currPlayer = !currPlayer;
		calc();
		if (allZero()) //我方也没得可下
		{
			repent();
			return pieceCnt[playerBfSim] > pieceCnt[!playerBfSim] ? oo/10 : -oo/10;
		}
		else //我方有的可下，依旧是我方下
		{
			repent();
			return oo/10;
		}
	}
	if (pieceCnt[0]+pieceCnt[1] == 64)
		return pieceCnt[playerBfSim] > pieceCnt[!playerBfSim] ? oo / 10 : -oo / 10;

	//非特例，常规MinMax搜索一层
	int ret = (depth%2) ? -oo : oo;
	for (int i = 1; i <= 8; ++i)
		for (int j = 1; j <= 8; ++j)
			if (calcBoard[i][j])
			{
				if (depth % 2) //Max层
					ret = max(ret, LeavePieceSimulate(i, j, depth + 1));
				else //Min层
					ret = min(ret, LeavePieceSimulate(i, j, depth + 1));
			}
	//对方如果没得可下就会返回-oo，这是对我方最有利的。
	repent(); //撤销这步模拟
	//cout << depth << " " << x << " " << y << " " << ret << endl;
	return ret;
}
void LeavePieceAccValueMedium()
{
	bestValue = -oo;
	int x, y;
	playerBfSim = currPlayer; //记录当前执子的玩家
	for (int i = 1; i <= 8; ++i)
		for (int j = 1; j <= 8; ++j)
			if (calcBoard[i][j])
			{
				int newBoardValue_ij = LeavePieceSimulate(i, j, 0);
				currPlayer = playerBfSim;
				if (newBoardValue_ij >= bestValue)
				{
					bestValue = newBoardValue_ij;
					x = i, y = j;
				}
			}
	leavePiece(x, y);
	Reverse(x, y);
	//system("pause");
	return;
}

void AISuggestion()
{
	int temp = searchDepth;
	searchDepth = 4;
	bestValue = -oo;
	int x, y;
	playerBfSim = currPlayer; //记录当前执子的玩家
	for (int i = 1; i <= 8; ++i)
		for (int j = 1; j <= 8; ++j)
			if (calcBoard[i][j])
			{
				int newBoardValue_ij = LeavePieceSimulate(i, j, 0);
				currPlayer = playerBfSim;
				if (newBoardValue_ij >= bestValue)
				{
					bestValue = newBoardValue_ij;
					x = i, y = j;
				}
			}
	cout << "AI suggests to leave piece on " << x << " " << y << endl;
	searchDepth = temp;
	return;
}

int personLeavePiece()
{
	int x, y;
	while (cin >> x >> y)
	{
		if (x == -2 && y == -2 && suggestionMood)
		{
			AISuggestion();
			continue;
		}
		if (x == -1 && y == -1)
			return 0;
		if (!x && !y && repentance && pieceCnt[0] + pieceCnt[1] != 4)
			return -1;
		if (!(x > 0 && x < 9 && y > 0 && y < 9))
		{
			cout << "Error input. Please try again." << endl;
			continue;
		}
		if (!calcBoard[x][y])
			cout << "Error input. Please try again." << endl;
		else
			break;
	}
	leavePiece(x, y);
	Reverse(x, y);
	return 1;
}

void endGame()
{
	printBoard();
	cout << endl;
	cout << "Game Over. " << endl;
	if (pieceCnt[0] == pieceCnt[1])
		cout << "TIE." << endl;
	else
		cout << "The WINNING SIDE is: " << numToColor(pieceCnt[0] > pieceCnt[1] ? 0 : 1) << ". " << endl;
	if (!currOppo && pveSide != (pieceCnt[0] > pieceCnt[1] ? 0 : 1))
		cout << "Don't lose your heart. Keep going! " << endl;
	else
		cout << "CONGRATES!!" << endl;
	cout << "Press enter to return to the menu." << endl;
	system("pause");
	return;
}

void saveProgress()
{
	fileStatus = 1;
	ofstream of("data.txt", ios::out);
	for (int i = 1; i <= 8; ++i)
	{
		for (int j = 1; j <= 8; ++j)
			of << realBoard[i][j] << " ";
		of << endl;
	}
	//calc();
	of << currPlayer << " " << currOppo << " " << diffLevel << " " << pveSide << " " << searchDepth << " " << pieceCnt[0] << " " << pieceCnt[1] << endl;
	int step = pieceCnt[0] + pieceCnt[1];
	for (int i = 5; i <= step; ++i)
	{
		of << currChess[i].player << " " << currChess[i].x << " " << currChess[i].y << endl;
		int size = currChess[i].cnt;
		of << size << endl;
		for (int j = 0; j < size; ++j)
		{
			of << currChess[i].rx[currChess[i].cnt] << " " << currChess[i].ry[currChess[i].cnt] << " ";
			currChess[i].cnt--;
		}
		of << endl;
	}
	of << A1 << " " << A2 << " " << A3 << endl;
	of.close();
	return;
}

void startGame()
{
	if (!currOppo && currPlayer == !pveSide) //该机器下
	{
		printBoard();
		cout << "Thinking..." << endl;
		if (diffLevel < 3)
			Sleep(1000);
		if (diffLevel == -1)
			LeavePieceAccNumEaten();
		else if (!diffLevel)
			LeavePieceAccValueEasy();
		else
		{
			searchDepth = 2 * diffLevel;
			LeavePieceAccValueMedium();
		}
	}
	else
	{
		while (true)
		{
			printBoard();
			cout << "Now it's " << numToColor(currPlayer) << " side's round. Please leave your piece. Or, you can try: " << endl << endl;
			cout << "        \"-1 -1\" Return to the menu & Record current process" << endl;
			if (suggestionMood)
				cout << "                   \"-2 -2\" AI Suggestion." << endl;
			if (repentance && pieceCnt[0]+pieceCnt[1] != 4) //悔棋开启且非初始步
				cout << "                       \"0 0\" Repent" << endl;
			cout << endl;
			int condition = personLeavePiece();
			if (!condition)
			{
				saveProgress();
				return;
			}
			else if (condition == -1)
			{
				bool tempCurrPlayer = currPlayer;
				if (!currOppo)
				{
					repent();
					while (currPlayer != tempCurrPlayer)
						repent();
				}//保证至少撤掉人的一步
				else
					repent(); //对手是人，悔1次，否则悔2次
			}
			else
				break;
		}
	}
	currPlayer = !currPlayer;
	calc();
	if (allZero()) //没得可下
	{
		currPlayer = !currPlayer;
		calc();
		if (allZero()) //我方也没得可下
		{
			endGame();
			return;
		}
		else //我方有的可下，依旧是我方下
		{
			cout << "The " << numToColor(!currPlayer) << " side has nowhere to leave the piece." << endl;
			cout << "Now it's still " << numToColor(currPlayer) << " side's round." << endl;
			system("pause");
		}
	}
	if (pieceCnt[0] + pieceCnt[1] == 64)
	{
		endGame();
		return;
	}
	else
		startGame();
	return;
}

void initialization()
{
	memset(realBoard, -1, sizeof(realBoard));
	currPlayer = 1;
	realBoard[4][5] = realBoard[5][4] = 1;
	realBoard[4][4] = realBoard[5][5] = 0;
	pieceCnt[0] = pieceCnt[1] = 2;
	calc();
	return;
}

void settings()
{
	while (true)
	{
		cout << "1 Permit repentance: " << boolToStr(repentance) << "." << endl;
		cout << "2 Permit suggestion: " << boolToStr(suggestionMood) << "." << endl;
		cout << endl << "Which one would you like to change? Type -1 to return to the menu." << endl;
		int change;
		cin >> change;
		if (change == -1)
			break;
		if (change == 1)
		{
			repentance = !repentance;
			system("cls");
			cout << "Change succeed." << endl;
		}
		else if (change == 2)
		{
			suggestionMood = !suggestionMood;
			system("cls");
			cout << "Change succeed." << endl;
		}
		else
		{
			system("cls");
			cout << "Invalid input. Please try again." << endl;
		}
	}
	return;
}

void readProgress()
{
	ifstream inf("data.txt", ios::in);
	for (int i = 1; i <= 8; ++i)
		for (int j = 1; j <= 8; ++j)
			inf >> realBoard[i][j];
	inf >> currPlayer >> currOppo >> diffLevel >> pveSide >> searchDepth >> pieceCnt[0] >> pieceCnt[1];
	int step = pieceCnt[0] + pieceCnt[1];
	for (int i = 5; i <= step; ++i)
	{
		inf >> currChess[i].player >> currChess[i].x >> currChess[i].y;
		inf >> currChess[i].cnt;
		for (int j = 1; j <= currChess[i].cnt; ++j)
			inf >> currChess[i].rx[j] >> currChess[i].ry[j];
	}
	inf >> A1 >> A2 >> A3;
	inf.close();
	return;
}

void intoMenu()
{
	system("cls");
	cout << "                          Reversi                            " << endl << endl;
	cout << "                            Menu                             " << endl;
	cout << "                        1 New Game                           " << endl;
	cout << "                        2 Read File                          " << endl;
	cout << "                        3 Settings                           " << endl;
	cout << "                        4 Exit                               " << endl << endl;
	cout << "Developer: Andy Lee(李安齐), Yuanpei College, Peking University." << endl;
	cout << "Please type in a number, then press Enter to start. Have fun! " << endl << endl;

	int starter;
	cin >> starter;
	system("cls");
	switch (starter)
	{
	case 2:
		if (!fileStatus)
		{
			cout << "                   No valid file." << endl << endl;
			cout << "              -1 Return to the Menu" << endl << endl;
			int x;
			cin >> x;
			intoMenu();
			break;
		}
		cout << "  Are you sure you want to continue the previous game? " << endl << endl;
		cout << "                     1 Sure." << endl;
		cout << "          0 No. Please start a new game." << endl;
		cout << "              -1 Return to the menu" << endl;
		int confirm;
		cin >> confirm;
		if (confirm == 1)
		{
			readProgress();
			startGame();
			intoMenu();
			break;
		}//否则不break，直接进入case1
		if (confirm == -1)
		{
			intoMenu();
			break;
		}
		system("pause");
	case 1:
		cout << "           Please select your opponent." << endl << endl;
		cout << "                  0 AI Player" << endl;
		cout << "                1 Your Partner" << endl;
		cout << "             -1 Return to the menu" << endl << endl;
		int typein;
		cin >> typein;
		if (typein == -1)
		{
			intoMenu();
			break;
		}
		currOppo = typein;
		if (!currOppo)
		{
			system("cls");
			cout << "         Please select your side." << endl << endl;
			cout << "               1 BLACK ○" << endl;
			cout << "               0 WHITE ●" << endl;
			cout << "          -1 Return to the menu" << endl << endl;
			cin >> typein;
			if (typein == -1)
			{
				intoMenu();
				break;
			}
			pveSide = typein;
			system("cls");
			cout << "Please choose the difficulty level of AI player." << endl << endl;
			cout << "              0 Beginner" << endl;
			cout << "              1 Novice" << endl;
			cout << "              2 Intermediate" << endl;
			cout << "              3 Expert" << endl;
			cout << "              4 Master" << endl;
			cout << "        -1 Return to the menu" << endl << endl;
			cin >> typein;
			if (typein == -1)
			{
				intoMenu();
				break;
			}
			diffLevel = typein - 1; //后三个难度需要与搜索深度的1/2对应，故减一。
		}
		initialization();
		startGame();
		intoMenu();
		break;
	case 3:
		settings();
		intoMenu();
		break;
	case 4:
		return;
	}
	return;
}

int main()
{
	intoMenu();
	return 0;
}