//---------------------------------------------------------------------
//---------------------------------------------------------------------
/*
 *  Copyright (c) Mikael Fridenfalk. All rights reserved.
 * 
 *  Redistribution and use in source and binary forms, with or 
 *  without modification, is hereby granted without fee provided 
 *  that the following conditions are met:
 * 
 *    1.  Redistributions of source code must retain the above 
 *        copyright notice, this list of conditions and the 
 *        following disclaimer.
 *    2.  Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the 
 *        following disclaimer in the documentation and/or other 
 *        materials provided with the distribution.
 *    3.  Neither the name of Mikael Fridenfalk may be used to
 *        endorse or promote products derived from this software
 *        without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY MIKAEL FRIDENFALK 'AS IS' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTIBILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MIKAEL
 * FRIDENFALK BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */
//---------------------------------------------------------------------
//---------------------------------------------------------------------
#include "Logic.h"
//---------------------------------------------------------------------
//---------------------------------------------------------------------
Logic::Logic(){
    mGamePos = new GAME_POS[MAX_PLY];
    mMove    = new MOVE[MAX_PLY];
    mMaxValue = 1e10;
    if (GAME == CHESS || GAME == REVERSI) return;
    mGamePosStatus = new GAME_POS[MAX_PLY];
    mGamePosST     = new GAME_POS_ST[MAX_PLY];
    mST_PreSet_GamePosIdx = 0;
}
//---------------------------------------------------------------------
//---------------------------------------------------------------------
Logic::~Logic(){
    delete [] mGamePos, delete [] mMove;
    if (GAME == CHESS || GAME == REVERSI) return;
    delete [] mGamePosStatus, delete [] mGamePosST;
}
//---------------------------------------------------------------------
//---------------------------------------------------------------------
void Logic::InitChess(){
    //------------------------------
    if (TEST1) Test_King_IsInCheck();
    if (TEST2) Test_OptChildren();
    //------------------------------
    mDeltaPly        =  4;
    mRandAmplitude   = .1;
    mOffensiveFactor = .1;
    //------------------------------Init Piece Values
    mPieceValue['*'] =    0.;
    mPieceValue['K'] = 1000.; mPieceValue['k'] = - mPieceValue['K'];
    mPieceValue['Q'] =    9.; mPieceValue['q'] = - mPieceValue['Q'];
    mPieceValue['R'] =    5.; mPieceValue['r'] = - mPieceValue['R'];
    mPieceValue['N'] =    3.; mPieceValue['n'] = - mPieceValue['N'];
    mPieceValue['B'] =    3.; mPieceValue['b'] = - mPieceValue['B'];
    mPieceValue['P'] =    1.; mPieceValue['p'] = - mPieceValue['P'];
    //------------------------------Square Values
    const float fileGrad[] = {.0, .1, .2, .3, .3, .2, .1, .0};
    For (x,FILES) mFileGrad[x] = fileGrad[x];
    For (y,RANKS) mRankGrad[y] = mOffensiveFactor * (float(y) - 3.5);
    //------------------------------Start Game Position
//  const char START[][16] = {///TMP
//        "* * * * * * * k",
//        "* * * * * * * *",
//        "* * * * * * * *",
//        "* * * * * * * *",
//        "* * * * * * * *",
//        "* * * * * * * *",
//        "* * * * * * R *",
//        "* * * * K R * *"
//    };
    //------------------------------Start Game Position
    const char START[][16] = {
        "r n b q k b n r",
        "p p p p p p p p",
        "* * * * * * * *",
        "* * * * * * * *",
        "* * * * * * * *",
        "* * * * * * * *",
        "P P P P P P P P",
        "R N B Q K B N R"
    };
    //------------------------------
    //   x, y : coordinates used locally in this class
    //   
    //   Ranks y : 0-7
    //   ^
    //   |
    //   |
    //   *-----> Files x : 0-7
    //   
    //   mGamePos[ply].sq[x][y]
    //------------------------------CHESS2
    /* const char START[][20] = {
        "r n n b q k b n n r",
        "p p p p p p p p p p",
        "* * * * * * * * * *",
        "* * * * * * * * * *",
        "* * * * * * * * * *",
        "* * * * * * * * * *",
        "* * * * * * * * * *",
        "* * * * * * * * * *",
        "P P P P P P P P P P",
        "R N N B Q K B N N R"
    }; */
    //------------------------------
    For (x,FILES) For (y,RANKS)
    mGamePos[0].sq[x][y] = START[RANKS_1-y][2*x];
    //------------------------------
    mPromotionFlags = 0;//For implementation of promotion
    //------------------------------
}
//---------------------------------------------------------------------
//---------------------------------------------------------------------
















//---------------------------------------------------------------------
//---------------------------------------------------------------------
//                          Computer Play
//---------------------------------------------------------------------
//---------------------------------------------------------------------
float Logic::FindBestMove(bool isWhite, int ply){
    //----------------------------------
    mN = mBestChildIdx = 0;
    mStartPly = ply;
    //----------------------------------
    mMaxPly = ply + mDeltaPly;
    //----------------------------------//STRATEGO 2
    if (GAME == STRATEGO && ST_SYS == IMPERF_INFO){
        ST2_BuildExpValMtx(DISP_OFF, mStartPly);
        mST2_VirtualPlay = true;
    } else mST2_VirtualPlay = false;
    //----------------------------------
    float score = CORE_ENGINE == MINIMAX ?
        Minimax(isWhite, ply) :
        AlphaBeta(-mMaxValue, mMaxValue, isWhite, ply);
    //----------------------------------
    mST2_VirtualPlay = false;//STRATEGO 2
    //----------------------------------
    mIsWhite = isWhite;//AlphaBeta may have changed mIsWhite
    mMove[ply].selectedChild = mBestChildIdx;
//  Print_LevelVerbose(ply);
//  printf("N = %d\n", mN);
    return score;
    //----------------------------------
}
//---------------------------------------------------------------------
//---------------------------------------------------------------------
//                             Minimax
//---------------------------------------------------------------------
//---------------------------------------------------------------------
float Logic::Minimax(bool isWhite, int ply){
    if (ply == mMaxPly) return GoalFunc(true, RAND_ON, ply);
    mIsWhite = isWhite; int N = FindChildren(ply);
    float alpha = isWhite ? -mMaxValue : mMaxValue;
    //----------------------------------
    if (GAME == REVERSI && N == 0) {
        Reversi_CopyBoard(ply, ply + 1);
        if (FindChildren(ply + 1) == 0)
            return GoalFunc(true, RAND_ON, ply);
        return Minimax(!isWhite, ply + 1);
    }
        
    //----------------------------------
    For (i, N){
        UpdateGamePos(i, ply+1);
        float score = Minimax(!isWhite, ply+1);
        if ((isWhite && alpha < score) || (!isWhite && alpha > score)){
            alpha = score;
            if (ply == mStartPly) mBestChildIdx = i;
        }
    }
    return alpha;
}
//---------------------------------------------------------------------
//---------------------------------------------------------------------
//                             AlphaBeta
//---------------------------------------------------------------------
//---------------------------------------------------------------------
float Logic::AlphaBeta(float a, float b, bool isWhite, int ply){
    if (ply == mMaxPly) return GoalFunc(true, RAND_ON, ply);
    mIsWhite = isWhite; int N = FindChildren(ply);
    //----------------------------------Check Mate?
    if (GAME == CHESS && N == 0 && mMove[ply].isInCheck)
        return isWhite ? mPieceValue['k'] : mPieceValue['K'];
    //----------------------------------
    if (GAME == REVERSI && N == 0) {
        Reversi_CopyBoard(ply, ply + 1);
        if (FindChildren(ply+1) == 0)
            return GoalFunc(true, RAND_ON, ply);
        return AlphaBeta(a, b, !isWhite, ply + 1);
    }
    //----------------------------------
    For (i, N){
        UpdateGamePos(i, ply+1);
        float score = AlphaBeta(a, b, !isWhite, ply+1);
        if (isWhite){if (a < score){
            a = score; if (ply == mStartPly) mBestChildIdx = i;}}
        else if (b > score){
            b = score; if (ply == mStartPly) mBestChildIdx = i;}
        if (a >= b) if (isWhite) return a; else return b;
    }
    if (isWhite) return a; else return b;
}
//---------------------------------------------------------------------
//---------------------------------------------------------------------
float Logic::GoalFunc(bool incrN, bool randOn, int ply){
    if (GAME == STRATEGO) return ST_GoalFunc(incrN, randOn, ply);
    if (GAME == REVERSI) return Reversi_GoalFunc(incrN, randOn, ply);
    float v = 0.;
    For (x,FILES) For (y,RANKS){
        char p = mGamePos[ply].sq[x][y];
        if (p != '*') v += mPieceValue[p] +
            mRankGrad[y] + (p < 'a' ? mFileGrad[x] : -mFileGrad[x]);
    } 
    if (incrN) mN++;
    return (randOn ? v + mRandAmplitude * Rand() : v);
}
//---------------------------------------------------------------------
//---------------------------------------------------------------------
float Logic::Rand(){//Standard C rand mapped to interval [-1,1[
    mRandSeed = mRandSeed * 1103515245 + 12345;
    int a = (unsigned int)(mRandSeed/65536) % 32768;
    return float(a)/16384.f - 1.f;
}
//---------------------------------------------------------------------
//---------------------------------------------------------------------
















//---------------------------------------------------------------------
//---------------------------------------------------------------------
//                            Human Play
//---------------------------------------------------------------------
//---------------------------------------------------------------------
bool Logic::SqBelongsToPlayer(char x, char y, bool isWhite, int ply){
    if (x < 0 || x > FILES_1 || y < 0 || y > RANKS_1) return false;
    char p = mGamePos[ply].sq[x][y];
    if (p == '*') return false;
    if (isWhite){if (p <= 'Z') return true;}//Same color (white)
    else if (p >= 'a') return true;//Same color (black)
    return false;
}
//---------------------------------------------------------------------
//---------------------------------------------------------------------
bool Logic::IsMoveOK_ManualPlay(char move[4], int ply){
    if (GAME == REVERSI) return Reversi_IsMoveOK_ManualPlay(move, ply);
    bool moveFound = false;
    FindChildren(ply);
    //Print_Level(ply);
    //---------------------------------------
    For (i, mMove[ply].children){
        if (mMove[ply].child[i][0] == move[0] &&
            mMove[ply].child[i][1] == move[1] &&
            mMove[ply].child[i][2] == move[2] &&
            mMove[ply].child[i][3] == move[3]){
            moveFound = true;
            break;
        }
    }
    //---------------------------------------
    if (moveFound){
        mMove[ply].selectedChild = 0;
        For (i,4) mMove[ply].child[0][i] = move[i];
    }
    //---------------------------------------
    mPromotionFlags = 0;//For implementation of pawn promotion
    return moveFound;
    //---------------------------------------
}
//---------------------------------------------------------------------
//---------------------------------------------------------------------
















//---------------------------------------------------------------------
//---------------------------------------------------------------------
//                               Aux
//---------------------------------------------------------------------
//---------------------------------------------------------------------
void Logic::UpdateGamePos(char childIdx, int ply){
    //--------------------------STRATEGO
    if (GAME == STRATEGO){ST_UpdateGamePos(childIdx, ply); return;}
    //--------------------------REVERSI
    if (GAME == REVERSI){Reversi_UpdateGamePos(childIdx, ply); return;}
    //--------------------------Simpler is better
    int D = ply - 1,
    x0 = mMove[D].child[childIdx][0], y0 = mMove[D].child[childIdx][1],
    x1 = mMove[D].child[childIdx][2], y1 = mMove[D].child[childIdx][3];
    //--------------------------Copy game position
    //mGamePos is a STRUCT, so the whole board is copied here,
    //not the pointer:
    mGamePos[ply] = mGamePos[D];
    //--------------------------Move piece in the copied game position
    mGamePos[ply].sq[x1][y1] = mGamePos[ply].sq[x0][y0];
    mGamePos[ply].sq[x0][y0] = '*';
    //--------------------------Chech for promotion in copied game pos
    char p = mGamePos[ply].sq[x1][y1];
    if (p == 'P' && y1 == RANKS_1) mGamePos[ply].sq[x1][y1] = 'Q';
    else if (p == 'p' && y1 == 0) mGamePos[ply].sq[x1][y1] = 'q';
    //The copied game position is updated
}
//---------------------------------------------------------------------
//---------------------------------------------------------------------
















//---------------------------------------------------------------------
//---------------------------------------------------------------------
//                           Find Children
//---------------------------------------------------------------------
//---------------------------------------------------------------------
int Logic::FindChildren(int ply){
    //---------------------------------Repetition Alert
    mRepetitionAlertOn = RepetitionAlert(ply);
    //---------------------------------STRATEGO
    if (GAME == STRATEGO) return ST_FindChildren(ply);
    //---------------------------------REVERSI
    if (GAME == REVERSI) return Reversi_FindChildren(ply);
    //---------------------------------Find Children
    mMove[ply].children = 0;
    For (x,FILES) For (y,RANKS){
        char p = mGamePos[ply].sq[x][y];
        if (p == '*') continue;//Empty square
        if (!mIsWhite) p -= 32;//Map ASCII-char lowercase to uppercase
        switch (p){
            case 'P': Pawn(x,y,ply);   break;
            case 'Q': Bishop(x,y,ply);//Queen = Bishop + Rook
            case 'R': Rook(x,y,ply);   break;
            case 'N': Knight(x,y,ply); break;
            case 'B': Bishop(x,y,ply); break;
            case 'K': King(x,y,ply);   break;
        }
    }
    if (mMove[ply].children == 0) return 0;
    
    //---------------------------------Move 2 best children to top   
    if (CORE_ENGINE == OPTIM && ply < mMaxPly-1){
        OptChildOrder(ply);
    }
    return mMove[ply].children;
    //---------------------------------
}
//---------------------------------------------------------------------
//---------------------------------------------------------------------
void Logic::OptChildOrder(int ply){
    float optimChildValue[MAX_CHILDREN_PER_PLY], score;
    int swapIdxs[2] = { 0, 1 };
    char tmp[4];
    For(i, mMove[ply].children) {
        UpdateGamePos(i, ply + 1);
        score = GoalFunc(true, RAND_OFF, ply + 1);
        optimChildValue[i] = mIsWhite ? score : -score;
        if (optimChildValue[i] > optimChildValue[swapIdxs[0]]) {
            swapIdxs[1] = swapIdxs[0];
            swapIdxs[0] = i;
        }
        else if (optimChildValue[i] > optimChildValue[swapIdxs[1]] &&
            optimChildValue[i] != optimChildValue[swapIdxs[0]]) {
            swapIdxs[1] = i;
        }
    }
    For(i, 2) For(m, 4) {
        tmp[m] = mMove[ply].child[i][m];
        mMove[ply].child[i][m] = mMove[ply].child[swapIdxs[i]][m];
        mMove[ply].child[swapIdxs[i]][m] = tmp[m];
    }
}
//---------------------------------------------------------------------
//---------------------------------------------------------------------
int Logic::MoveStatus(int x, int y, int ply){
    //return 0: Invalid - piece of same color or square outside board
    //return 1: Probably valid - empty square
    //return 2: Probably valid - piece of opposite color <=> capture
    if (x < 0 || x > FILES_1 || y < 0 || y > RANKS_1) return 0;
    if (GAME == STRATEGO && ST_PositionIsWater(x,y)) return 0;
    char piece = mGamePos[ply].sq[x][y];
    if (piece == '*') return 1;
    if (mIsWhite){if (piece <= 'Z') return 0;}//Same color (white)
    else if (piece >= 'a') return 0;//Same color (black)
    return 2;
}
//---------------------------------------------------------------------
//---------------------------------------------------------------------
void Logic::SuggestAddChild(int x0, int y0, int x1, int y1, int ply){
    int N = mMove[ply].children;
    mMove[ply].child[N][0] = x0; mMove[ply].child[N][1] = y0;
    mMove[ply].child[N][2] = x1; mMove[ply].child[N][3] = y1;
    //A child is added, unless move is invalid due to repetition
    if (mRepetitionAlertOn &&
        mInvalidRepetitiveMove[0] == x0 &&
        mInvalidRepetitiveMove[1] == y0 &&
        mInvalidRepetitiveMove[2] == x1 &&
        mInvalidRepetitiveMove[3] == y1){
        mRepetitionAlertOn = false;
        return;
    }
    if (GAME == STRATEGO || GAME == REVERSI){
        mMove[ply].children++; return;
    }//STRATEGO or REVERSI
    //Child is added, unless king is in check
    UpdateGamePos(N, ply+1);
    if (!King_IsInCheck(ply+1)) mMove[ply].children++;
}
//---------------------------------------------------------------------
//---------------------------------------------------------------------
void Logic::Pawn(int x, int y, int ply){
    const char one[] = {-1, 1}, two[] = {-2, 2}, start[] = {1, 6};
    int w = (int)mIsWhite, y1 = y + one[w], y2 = y + two[w];
    //Push Forward
    if (MoveStatus(x,y+one[w],ply) == 1){
        SuggestAddChild(x,y,x,y1,ply);//1 step
        if (y == start[1-w] && MoveStatus(x,y2,ply) == 1)//2 steps
            SuggestAddChild(x,y,x,y2,ply);
    }
    //Capture - x direction
    if (MoveStatus(x-1,y+one[w],ply) == 2)
        SuggestAddChild(x,y,x-1,y1,ply);
    //Capture + x direction
    if (MoveStatus(x+1,y+one[w],ply) == 2)
        SuggestAddChild(x,y,x+1,y1,ply);
}
//---------------------------------------------------------------------
//---------------------------------------------------------------------
void Logic::Rook(int x, int y, int ply){
    //The scout in Stratego also uses this method
    int x1, y1;
    For (k,2) For (i,FILES_1){
        x1 = k ? x + i + 1 : x - i - 1;
        int a = MoveStatus(x1,y,ply);
        if (a == 0) break;
        SuggestAddChild(x,y,x1,y,ply);
        if (a == 2) break;
    }
    For (k,2) For (i,RANKS_1){
        y1 = k ? y + i + 1 : y - i - 1;
        int a = MoveStatus(x,y1,ply);
        if (a == 0) break;
        SuggestAddChild(x,y,x,y1,ply);
        if (a == 2) break;
    }
}
//---------------------------------------------------------------------
//---------------------------------------------------------------------
void Logic::Knight(int x, int y, int ply){
    const int moves[][2] = {
        1,2,  2,1,  1,-2,  2,-1,  -1,2,  -2,1,  -1,-2,  -2,-1};
    For (i,8){
        int x1 = x + moves[i][0], y1 = y + moves[i][1];
        if (MoveStatus(x1,y1,ply) > 0)
            SuggestAddChild(x,y,x1,y1,ply);
    }
}
//---------------------------------------------------------------------
//---------------------------------------------------------------------
void Logic::Bishop(int x, int y, int ply){
    int x1, y1;
    For (k,2) For (i,DIAGS_1){
        x1 = x + i + 1;
        y1 = k ? y + i + 1 : y - i - 1;
        int a = MoveStatus(x1,y1,ply);
        if (a == 0) break;
        SuggestAddChild(x,y,x1,y1,ply);
        if (a == 2) break;
    }
    For (k,2) For (i,DIAGS_1){
        x1 = x - i - 1;
        y1 = k ? y + i + 1 : y - i - 1;
        int a = MoveStatus(x1,y1,ply);
        if (a == 0) break;
        SuggestAddChild(x,y,x1,y1,ply);
        if (a == 2) break;
    }
}
//---------------------------------------------------------------------
//---------------------------------------------------------------------
//                         Repetition Alert
//---------------------------------------------------------------------
//---------------------------------------------------------------------
bool Logic::RepetitionAlert(int ply){
    
    if (ply < 10) return false;
    
    //square1   square2
    //ply N-10:      i         *
    //ply N-8:       *         i
    //ply N-6:       i         *
    //ply N-4:       *         i
    //ply N-2:       i         *
    //ply N:         *         i => threefold repetition alert
    
    if (IsMoveUnequal(ply - 6, ply - 10) ||
        IsMoveUnequal(ply - 4, ply -  8) ||
        IsMoveUnequal(ply - 2, ply -  6)) return false;
    int d_4 = ply - 4;
    int c = mMove[d_4].selectedChild;
    For (m,4) mInvalidRepetitiveMove[m] = mMove[d_4].child[c][m];
    return true;
}
//---------------------------------------------------------------------
//---------------------------------------------------------------------
bool Logic::IsMoveUnequal(int depth0, int depth1){
    int c0 = mMove[depth0].selectedChild;
    int c1 = mMove[depth1].selectedChild;
    For (m,4) if (mMove[depth0].child[c0][m] !=
                  mMove[depth1].child[c1][m]) return true;
    return false;
}
//---------------------------------------------------------------------
//---------------------------------------------------------------------
















//---------------------------------------------------------------------
//---------------------------------------------------------------------
//                               King
//---------------------------------------------------------------------
//---------------------------------------------------------------------
void Logic::King(int x, int y, int ply){
    const int moves[][2] = {
        1,0, 1,1, 0,1, -1,1, -1,0, -1,-1, 0,-1, 1,-1};
    For (i,8){
        int x1 = x + moves[i][0], y1 = y + moves[i][1];
        if (MoveStatus(x1,y1,ply) > 0)
            SuggestAddChild(x,y,x1,y1,ply);
    }
}
//---------------------------------------------------------------------
//---------------------------------------------------------------------
bool Logic::King_IsInCheck(int ply){
    
    //--------------------------------------
    if (GAME == STRATEGO) return false;
    //--------------------------------------
    if (GAME == REVERSI) return false;
    //--------------------------------------
    
    int x, y, a, w = (int)mIsWhite;
    char K[2] = { 'K', 'k' },
         Q[2] = { 'Q', 'q' },
         R[2] = { 'R', 'r' },
         N[2] = { 'N', 'n' },
         B[2] = { 'B', 'b' },
         P[2] = { 'P', 'p' };

    //   (A) Locate king
    For(i, FILES) For(j, RANKS)
        if (K[1 - w] == mGamePos[ply].sq[i][j]) {
            x = i; y = j; i = FILES; j = RANKS;//double break
        }
    mKingPos[0] = x, mKingPos[1] = y;

    //   (B) return true if king is in check by opponent's pawn
    const int pCaptureDir[] = { -1, 1 };
    y = mKingPos[1] + pCaptureDir[w];
    For(i, 2) {
        x = mKingPos[0] + pCaptureDir[i];
        a = MoveStatus(x, y, ply);
        if (a == 2 && mGamePos[ply].sq[x][y] == P[w])
            return true;
    }

    //   (C) return true if king is in check by opponent's king
    const int kMoves[][2] = {
        1,0, 1,1, 0,1, -1,1, -1,0, -1,-1, 0,-1, 1,-1 };
    For(i, 8) {
        x = mKingPos[0] + kMoves[i][0], y = mKingPos[1] + kMoves[i][1];
        a = MoveStatus(x, y, ply);
        if (a == 2 && mGamePos[ply].sq[x][y] == K[w])
            return true;
    }

    //   (D) return true if king is in rank or file check
    //       (opponent's queen or rook)
    For(k, 2) For(i, FILES_1) {
        x = k ? mKingPos[0] + i + 1 : mKingPos[0] - i - 1;
        a = MoveStatus(x, mKingPos[1], ply);
        if (a == 0) break;
        if (a == 2) {
            if (mGamePos[ply].sq[x][mKingPos[1]] == Q[w] ||
                mGamePos[ply].sq[x][mKingPos[1]] == R[w])
                return true;
            break;
        }
    }
    For(k, 2) For(i, RANKS_1) {
        y = k ? mKingPos[1] + i + 1 : mKingPos[1] - i - 1;
        a = MoveStatus(mKingPos[0], y, ply);
        if (a == 0) break;
        if (a == 2) {
            if (mGamePos[ply].sq[mKingPos[0]][y] == Q[w] ||
                mGamePos[ply].sq[mKingPos[0]][y] == R[w])
                return true;
            break;
        }
    }

    //   (E) return true if king is in diagonal check
    //       (opponent's queen or bishop)
    For(k, 2) For(i, DIAGS_1) {
        x = mKingPos[0] + i + 1;
        y = k ? mKingPos[1] + i + 1 : mKingPos[1] - i - 1;
        a = MoveStatus(x, y, ply);
        if (a == 0) break;
        if (a == 2) {
            if (mGamePos[ply].sq[x][y] == Q[w] ||
                mGamePos[ply].sq[x][y] == B[w])
                return true;
            break;
        }
    }
    For(k, 2) For(i, DIAGS_1) {
        x = mKingPos[0] - i - 1;
        y = k ? mKingPos[1] + i + 1 : mKingPos[1] - i - 1;
        a = MoveStatus(x, y, ply);
        if (a == 0) break;
        if (a == 2) {
            if (mGamePos[ply].sq[x][y] == Q[w] ||
                mGamePos[ply].sq[x][y] == B[w])
                return true;
            break;
        }
    }

    //   (F) return true if king is in check by opponent's knight
    const int nMoves[][2] = {
        1,2, 2,1, 1,-2, 2,-1, -1,2, -2,1, -1,-2, -2,-1 };
    For(i, 8) {
        x = mKingPos[0] + nMoves[i][0], y = mKingPos[1] + nMoves[i][1];
        a = MoveStatus(x, y, ply);
        if (a == 2 && mGamePos[ply].sq[x][y] == N[w])
            return true;
    }

    //   (G) return false
    return false;
}
//---------------------------------------------------------------------
//---------------------------------------------------------------------
















//---------------------------------------------------------------------
//---------------------------------------------------------------------
//                              Print
//---------------------------------------------------------------------
//---------------------------------------------------------------------
// We use printf throughout this book instead of cout, since printf
// uses less memory and prints faster than cout...
//---------------------------------------------------------------------
//---------------------------------------------------------------------
void Logic::Print_Board(bool includeIdx, int ply){
    For (y,RANKS){
        printf(" ");
        if (includeIdx) printf("%2d",RANKS-y);
        For (x,FILES) printf(" %c",mGamePos[ply].sq[x][RANKS_1-y]);
        printf("\n");
    }
    if (includeIdx){
        printf("    ");
        For (x,FILES) printf("%c ",65+x);
    }
    if (includeIdx) printf("\n");
}
//---------------------------------------------------------------------
//---------------------------------------------------------------------
void Logic::Print_Child(int childIdx, int ply){
    char a = mMove[ply].child[childIdx][0] + 'a';
    char b = mMove[ply].child[childIdx][1] +  1 ;
    char c = mMove[ply].child[childIdx][2] + 'a';
    char d = mMove[ply].child[childIdx][3] +  1 ;
    if (RANKS > 9) printf("%c%2.2d%c%2.2d", a, b, c, d);
    else printf("%c%d%c%d", a, b, c, d);
}
//---------------------------------------------------------------------
//---------------------------------------------------------------------
void Logic::Print_Level(int ply){
    // Warning! If mRandAmplitude > 0. the value of goal function
    // will fluctuate at print
    bool isWhite = !bool(ply%2);
    printf("-------------------------- PLY %d ",ply);
    if (isWhite) printf("(WHITE'S TURN)\n");
    else printf("(BLACK'S TURN)\n");
    Print_Board(true, ply);
    printf("-------------------------- ");
    if (isWhite) printf("Possible moves for WHITE");
    else printf("Possible moves for BLACK");
    For (i,mMove[ply].children){
        if ((i%8) == 0) printf("\n[%2.2d]",i);
        printf(" ");
        Print_Child(i,ply);
    }
    printf("\n");
    For (i,4) printf("--------------------------\n");
}
//---------------------------------------------------------------------
//---------------------------------------------------------------------
void Logic::Print_LevelVerbose(int ply){
    bool isWhite = !bool(ply%2);
    printf("-------------------------- PLY %d ",ply);
    if (isWhite) printf("(WHITE'S TURN)\n");
    else printf("(BLACK'S TURN)\n");
    Print_Board(true, ply);
    printf("-------------------------- ");
    if (isWhite) printf("Possible moves for WHITE");
    else printf("Possible moves for BLACK");
    For (i, mMove[ply].children){
        printf("\n %2.2d  ",i);
        Print_Child(i,ply);
        UpdateGamePos(i,ply+1);
        float v = GoalFunc(false, RAND_OFF, ply+1);
        printf("  %6.2f",v);
        if (i == mBestChildIdx){
            if (isWhite) printf(" < MAX");
            else printf(" < MIN");
        }
    }
    printf("\n");
    For (i,4) printf("--------------------------\n");
}
//---------------------------------------------------------------------
//---------------------------------------------------------------------
void Logic::Print_Bar(){
    printf("--------------------------------------------------"//50
           "----------------------\n");//22
           //"------\n");//6
}
//---------------------------------------------------------------------
//---------------------------------------------------------------------
















//---------------------------------------------------------------------
//---------------------------------------------------------------------
//                   STRATEGO - PERFECT INFO AI
//---------------------------------------------------------------------
//---------------------------------------------------------------------
void Logic::ST_Init(){//STRATEGO
    //------------------------------
    if (TEST3) Test_ST2_BasicStats();
    //------------------------------
    ST_BuildDst2SrcMtx();
    //------------------------------
    mDeltaPly        =  6;
    mRandAmplitude   = .1;
    mOffensiveFactor = .2;
    //INIT-PIECE-VALUES-------------------------------RANK#-----#-
    mGamePosST[0].pieceValue['*'] =    0.;//Empty       -       -
    mGamePosST[0].pieceValue['A'] =   50.;//Flag        0       1
    mGamePosST[0].pieceValue['B'] =   10.;//Spy         1       1
    mGamePosST[0].pieceValue['C'] =    1.;//Scout       2       8
    mGamePosST[0].pieceValue['D'] =    8.;//Miner       3       5
    mGamePosST[0].pieceValue['E'] =    3.;//Sergeant    4       4       
    mGamePosST[0].pieceValue['F'] =    4.;//Lieutenant  5       4       
    mGamePosST[0].pieceValue['G'] =    5.;//Captain     6       4       
    mGamePosST[0].pieceValue['H'] =    7.;//Major       7       3       
    mGamePosST[0].pieceValue['I'] =    9.;//Colonel     8       2       
    mGamePosST[0].pieceValue['J'] =   12.;//General     9       1
    mGamePosST[0].pieceValue['K'] =   20.;//Marshal    10       1
    mGamePosST[0].pieceValue['L'] =   10.;//Trap       11       6
    //------------------------------------------------------------
    For (i,12) mGamePosST[0].pieceValue[i+'a'] =
        - mGamePosST[0].pieceValue[i+'A'];
    //------------------------------------------------------------
    if (ST_SYS == PERF_INFO) mGamePosST[0].deltaExpVal = 0.f;
    //------------------------------Start Game Position
    const char START[][20] = {
        "f c e g d d l a l c",
        "c f l f e d g l d g",
        "c h d j e i e b c i",
        "h c h g l c f k c l",
        "* * * * * * * * * *",
        "* * * * * * * * * *",
        "L C K F C L G H C H",
        "I C B E I E J D H C",
        "G D L G D E F L F C",
        "C L A L D D G E C F",
    };
    //------------------------------
    //   x, y : coordinates used locally in this class
    //   
    //   Ranks y : 0-9
    //   ^
    //   |
    //   |
    //   *-----> Files x : 0-9
    //   
    //   mGamePos[ply].sq[x][y]
    //------------------------------Square Values
    For (x,FILES) mFileGrad[x] = 0.;
    For (y,RANKS) mRankGrad[y] = mOffensiveFactor * (float(y) - 4.5);
    //------------------------------
    For (x,FILES) For (y,RANKS){
        mGamePos[0].sq[x][y] = START[RANKS_1-y][2*x];
        mGamePosStatus[0].sq[x][y] = SEALED;
    }
    //------------------------------
    //APPENDIX G & H
    if (ST2_SPECIAL_CASE) mGamePosStatus[0].sq[0][8] = MOBILE;//ST2
    //------------------------------
    For (i,3) mST_PieceStatusValue['*'][i] = 0.;
    ST_EvalPieceStatusValues(0);
    //------------------------------
    //APPENDIX G & H & I
    if (ST2_PRINT > ST2_PRINT_NONE) ST2_BuildExpValMtx(DISP_ON,0);//ST2
    //------------------------------
}
//---------------------------------------------------------------------
//---------------------------------------------------------------------
void Logic::ST_BuildDst2SrcMtx(){
    char S, s, D, d, p;
    //Move to empty square
    for (S = 'B'; S <= 'K'; S++) mST_CaptureMtx[S]['*'] = S;
    for (s = 'b'; s <= 'k'; s++) mST_CaptureMtx[s]['*'] = s;
    //Black attempt to capture white piece
    for (s = 'b'; s <= 'k'; s++)
        for (D = 'A'; D <= 'L'; D++){
            if ((S = s - 32) < D) p = D;
            else if (S == D) p = '*'; else p = s;
            mST_CaptureMtx[s][D] = p; 
            //printf("[%c&%c=%c]\n",s,D,p);
        }
    //White attempt to capture black piece
    for (S = 'B'; S <= 'K'; S++)
        for (d = 'a'; d <= 'l'; d++){
            if ((s = S + 32) < d) p = d;
            else if (s == d) p = '*'; else p = S;
            mST_CaptureMtx[S][d] = p; 
            //printf("[%c&%c=%c]\n",S,d,p);
        }
    //Execption 1: ST_Spy (B) beats Marshal (K)
    mST_CaptureMtx['B']['k'] = 'B';
    mST_CaptureMtx['b']['K'] = 'b';
    //Execption 2: ST_Miner (D) beats Mine (L)
    mST_CaptureMtx['D']['l'] = 'D';
    mST_CaptureMtx['d']['L'] = 'd';
}
//---------------------------------------------------------------------
//---------------------------------------------------------------------
void Logic::ST_EvalPieceStatusValues(int ply){
    //----------------------------------------
    const float
    SEALED_OFFSET = 1.0f, MOBILE_OFFSET = 0.5f,
    SEALED_FACTOR = 0.2f, MOBILE_FACTOR = 0.1f;
    //----------------------------------------
    For (i,12){
        mST_PieceStatusValue[i+'A'][SEALED] = SEALED_OFFSET +
        SEALED_FACTOR * mGamePosST[ply].pieceValue[i+'A'];
        
        mST_PieceStatusValue[i+'A'][MOBILE] = MOBILE_OFFSET +
        MOBILE_FACTOR * mGamePosST[ply].pieceValue[i+'A'];
        
        mST_PieceStatusValue[i+'A'][KNOWN]  = 0.;
        
        mST_PieceStatusValue[i+'a'][SEALED] =
        - mST_PieceStatusValue[i+'A'][SEALED];
        
        mST_PieceStatusValue[i+'a'][MOBILE] =
        - mST_PieceStatusValue[i+'A'][MOBILE];
        
        mST_PieceStatusValue[i+'a'][KNOWN]  =
        - mST_PieceStatusValue[i+'A'][KNOWN];
    }
    //----------------------------------------
}
//---------------------------------------------------------------------
//---------------------------------------------------------------------
//                Goal Function Addition - STRATEGO
//---------------------------------------------------------------------
//---------------------------------------------------------------------
float Logic::ST_GoalFunc(bool incrN, bool randOn, int ply){
    float v;
    if ((ST_SYS == IMPERF_INFO) && mST2_VirtualPlay)
        v = mGamePosST[ply].deltaExpVal;
    else {
        v = 0.;
        For (x,FILES) For (y,RANKS){
            char p = mGamePos[ply].sq[x][y];
            char s = mGamePosStatus[ply].sq[x][y];
            if (p != '*') v += mGamePosST[ply].pieceValue[p] +
                mST_PieceStatusValue[p][s] + mRankGrad[y];
        }
    }
    if (incrN) mN++;
    return (randOn ? v + mRandAmplitude * Rand() : v);
}
//---------------------------------------------------------------------
//---------------------------------------------------------------------
//                     Find Children - STRATEGO
//---------------------------------------------------------------------
//---------------------------------------------------------------------
int Logic::ST_FindChildren(int ply){
    
    if ((ST_SYS == IMPERF_INFO) && mST2_VirtualPlay)
        return ST2_FindChildren(ply);
    
    mMove[ply].children = 0;
    For (x,FILES) For (y,RANKS){
        char p = mGamePos[ply].sq[x][y];
        if (p == '*') continue;//Empty square
        if (!mIsWhite) p -= 32;//Map ASCII-char lowercase to uppercase
        if (p == 'C'){Rook(x,y,ply); continue;}
        if (p >= 'B' && p <= 'K') ST_BasicMoves(x,y,ply);
    }
    if (mMove[ply].children == 0) return 0;
    //---------------------------------Move best child to top of list   
    if (CORE_ENGINE == OPTIM && ply < mMaxPly-1){
        OptChildOrder(ply);
    }
    return mMove[ply].children;
    //---------------------------------
}
//---------------------------------------------------------------------
//---------------------------------------------------------------------
inline bool Logic::ST_PositionIsWater(int x, int y){
    if (y < 4 || y > 5) return false;
    return x == 2 || x == 3 || x == 6 || x == 7;
}
//---------------------------------------------------------------------
//---------------------------------------------------------------------
void Logic::ST_BasicMoves(int x, int y, int ply){
    const int moves[][2] = {1,0, -1,0, 0,1, 0,-1};
    For (i,4){
        int x1 = x + moves[i][0], y1 = y + moves[i][1];
        if (MoveStatus(x1,y1,ply) > 0) SuggestAddChild(x,y,x1,y1,ply);
    }
}
//---------------------------------------------------------------------
//---------------------------------------------------------------------
void Logic::ST_UpdateGamePos(char childIdx, int ply){
    
    if ((ST_SYS == IMPERF_INFO) && mST2_VirtualPlay){
        ST2_UpdateGamePos(childIdx, ply); return;}
    
    //--------------------------Short forms
    int D = ply - 1,
    x0 = mMove[D].child[childIdx][0], y0 = mMove[D].child[childIdx][1],
    x1 = mMove[D].child[childIdx][2], y1 = mMove[D].child[childIdx][3];
    //--------------------------Copy game position
    //mGamePos, mGamePosStatus and mGamePosST are a STRUCTS, so the
    //whole board is copied here, not the pointer:
    mGamePos[ply]       = mGamePos[D];
    mGamePosStatus[ply] = mGamePosStatus[D];
    mGamePosST[ply]     = mGamePosST[D];
    //--------------------------Move piece in the copied game position
    char p0 = mGamePos[ply].sq[x0][y0];
    char p1 = mGamePos[ply].sq[x1][y1];
    mGamePos[ply].sq[x0][y0] = '*';
    mGamePos[ply].sq[x1][y1] = mST_CaptureMtx[p0][p1];
    //--------------------------
    if (p1 == '*'){//If move to empty square
        mGamePosStatus[ply].sq[x1][y1] = 
        mGamePosStatus[ply].sq[x0][y0] != KNOWN ? MOBILE : KNOWN;
    }
    else mGamePosStatus[ply].sq[x1][y1] = KNOWN;
    //--------------------------
}
//---------------------------------------------------------------------
//---------------------------------------------------------------------
bool Logic::ST_IsFlagCaptured(bool targetIsWhiteFlag, int ply){
    char A = targetIsWhiteFlag ? 'A' : 'a';
    bool captured = true;
    For (x,FILES) For (y,RANKS) if (mGamePos[ply].sq[x][y] == A){
        captured = false;
        break;
    }
    return captured;
}
//---------------------------------------------------------------------
//---------------------------------------------------------------------
void Logic::ST_NewRand_GamePos_Start(bool isWhite){ 
    if (isWhite){
        ST_ShufflePiecePos(0, 1);
        ST_ShufflePiecePos(2, 3);
    }
    else {
        ST_ShufflePiecePos(6, 7);
        ST_ShufflePiecePos(8, 9);
    }
}
//---------------------------------------------------------------------
//---------------------------------------------------------------------
void Logic::ST_NewPreSet_GamePos_Start(bool isWhite){
    mST_PreSet_GamePosIdx = (mST_PreSet_GamePosIdx + 1) % 5;
    const char PRESETS[][10][20] = {
        "d c b i h l d e c e",
        "g e d g l a l k i g",
        "h f l g f l f e l f",
        "c d c j c c h d c c",
        "* * * * * * * * * *",
        "* * * * * * * * * *",
        "C C D H C C J C D C",
        "F L E F L F G L F H",
        "G I K L A L G D E G",
        "E C E D L H I B C D",

        "c g e e c c l a l e",
        "d c f g d f d l h j",
        "i l l f l d b i f c",
        "c h g g c h k d e c",
        "* * * * * * * * * *",
        "* * * * * * * * * *",
        "C E D K H C G G H C",
        "C F I B D L F L L I",
        "J H L D F D G F C D",
        "E L A L C C E E G C",

        "e d h c l a l d g d",
        "l f e d e l c b d g",
        "g c l h i c g e i h",
        "f c j f l c c k c f",
        "* * * * * * * * * *",
        "* * * * * * * * * *",
        "F C K C C L F J C F",
        "H I E G C I H L C G",
        "G D B C L E D E F L",
        "D G D L A L C H D E",

        "c g l d c c g e d c",
        "g h f l d f l f h b",
        "f d e c g d e i e k",
        "c i l h c j l a l c",
        "* * * * * * * * * *",
        "* * * * * * * * * *",
        "C L A L J C H L I C",
        "K E I E D G C E D F",
        "B H F L F D L F H G",
        "C D E G C C D L G C",

        "f c e g d d l a l c",
        "c f l f e d g l d g",
        "c h d j e i e b c i",
        "h c h g l c f k c l",
        "* * * * * * * * * *",
        "* * * * * * * * * *",
        "L C K F C L G H C H",
        "I C B E I E J D H C",
        "G D L G D E F L F C",
        "C L A L D D G E C F",
    };

    For(x, FILES) mFileGrad[x] = 0.;
    For(y, RANKS) mRankGrad[y] = mOffensiveFactor * (float(y) - 4.5);
    //------------------------------
    For(x, FILES) For(y, 4) {
        mGamePos[0].sq[x][isWhite ? y : RANKS_1 - y] =
            PRESETS[mST_PreSet_GamePosIdx][isWhite ? RANKS_1 - y : y][2 * x];
        mGamePosStatus[0].sq[x][y] = SEALED;
    }
    if (ST2_SPECIAL_CASE) mGamePosStatus[0].sq[0][8] = MOBILE;//ST2
    For(i, 3) mST_PieceStatusValue['*'][i] = 0.;
    ST_EvalPieceStatusValues(0);
    if (ST2_PRINT > ST2_PRINT_NONE) ST2_BuildExpValMtx(DISP_ON, 0);//ST2
}
//---------------------------------------------------------------------
//---------------------------------------------------------------------
void Logic::ST_ShufflePiecePos(int fromY, int toY) {
    For(i, 1000) {
        int posX1 = 10. * .5 * (1. + Rand());
        int posX2 = 10. * .5 * (1. + Rand());
        int dY = toY - fromY;
        int posY1 = fromY + dY * (1. + Rand());
        int posY2 = fromY + dY * (1. + Rand());
        char temp = mGamePos[0].sq[posX1][posY1];
        mGamePos[0].sq[posX1][posY1] = mGamePos[0].sq[posX2][posY2];
        mGamePos[0].sq[posX2][posY2] = temp;
    }  
}
//---------------------------------------------------------------------
//---------------------------------------------------------------------
















//---------------------------------------------------------------------
//---------------------------------------------------------------------
//                  STRATEGO 2 - IMPERFECT INFO AI 
//---------------------------------------------------------------------
//---------------------------------------------------------------------
void Logic::ST2_BuildExpValMtx(bool disp, int ply){
    
    if (disp){printf("\n"); Print_Bar(); Print_Bar();}
    
    //--------------------------------------------
    ST2_BasicStats(disp, ply);
    //--------------------------------------------
    enum {STRIKER_OPPONENT, STRIKER_PLAYER};
    //--------------------------------------------
    ST2_BuildExpValSubMtx(SEALED, SEALED, STRIKER_PLAYER, ply);//Scout
    ST2_BuildExpValSubMtx(MOBILE, SEALED, STRIKER_PLAYER, ply);
    ST2_BuildExpValSubMtx(KNOWN,  SEALED, STRIKER_PLAYER, ply);
    ST2_BuildExpValSubMtx(SEALED, MOBILE, STRIKER_PLAYER, ply);
    ST2_BuildExpValSubMtx(MOBILE, MOBILE, STRIKER_PLAYER, ply);
    ST2_BuildExpValSubMtx(KNOWN,  MOBILE, STRIKER_PLAYER, ply);
    //--------------------------------------------
    ST2_BuildExpValSubMtx(SEALED, SEALED, STRIKER_OPPONENT, ply);
    ST2_BuildExpValSubMtx(MOBILE, SEALED, STRIKER_OPPONENT, ply);
    ST2_BuildExpValSubMtx(KNOWN,  SEALED, STRIKER_OPPONENT, ply);
    ST2_BuildExpValSubMtx(SEALED, MOBILE, STRIKER_OPPONENT, ply);
    ST2_BuildExpValSubMtx(MOBILE, MOBILE, STRIKER_OPPONENT, ply);
    ST2_BuildExpValSubMtx(KNOWN,  MOBILE, STRIKER_OPPONENT, ply);
    //--------------------------------------------
    
    if (!disp) return;
    
    //--------------------------------------------
    //--------------------------------------------
    printf("\n\n");
    Print_Bar(); Print_Bar();
    //--------------------------------------------
    const char ST[3][16] = {"S","M","K"};
    printf(" PLAYER p0 = A...L (S = SEALED, M = MOBILE, K = KNOWN)\n");
    Print_Bar();
    //--------------------------------------------
    printf(" Success Probability: p0 -> p1\n");
    Print_Bar();
    printf(" p0    =  ");
    For (i,12) printf("  %c  ",i+'A');
    printf("\n");
    For (i,2){
        printf(" p0 -> %s ", ST[i]);
        For (p,12) printf("%5.2f", mST2_CaptureProbMtx[p+'A'][i][1]);
        printf("\n");
    }
    //--------------------------------------------
    Print_Bar();
    printf(" Expected Value: p0 -> p1\n");
    Print_Bar();
    printf(" p0    =  ");
    For (i,12) printf("  %c  ",i+'A');
    printf("\n");
    For (j,2) For (i,3){
        //if (2-i == SEALED && j == SEALED) continue;
        printf("  %s -> %s ", ST[2-i], ST[j]);
        For (p,12)
        printf("%5.1f",
               mST2_ExpValSuccessMtx[p+'A'][2-i][j][1] +
               mST2_ExpValFailureMtx[p+'A'][2-i][j][1]);
        printf("\n");
    }
    //--------------------------------------------
    Print_Bar();
    printf(" Expected Value at Success: p0 -> p1\n");
    Print_Bar();
    printf(" p0    =  ");
    For (i,12) printf("  %c  ",i+'A');
    printf("\n");
    For (j,2) For (i,3){
        //if (2-i == SEALED && j == SEALED) continue;
        printf("  %s -> %s ", ST[2-i], ST[j]);
        For (p,12)
            printf("%5.1f", mST2_ExpValSuccessMtx[p+'A'][2-i][j][1]);
        printf("\n");
    }
    //--------------------------------------------
    Print_Bar();
    printf(" Expected Value at Failure: p0 -> p1\n");
    Print_Bar();
    printf(" p0    =  ");
    For (i,12) printf("  %c  ",i+'A');
    printf("\n");;
    For (j,2) For (i,3){
        //if (2-i == SEALED && j == SEALED) continue;
        printf("  %s -> %s ", ST[2-i], ST[j]);
        For (p,12)
            printf("%5.1f", mST2_ExpValFailureMtx[p+'A'][2-i][j][1]);
        printf("\n");
    }
    Print_Bar(); Print_Bar();
    //--------------------------------------------
    //--------------------------------------------
    
    
    //--------------------------------------------
    //--------------------------------------------
    printf("\n\n");
    Print_Bar(); Print_Bar();
    //--------------------------------------------
    printf(" OPPONENT, with TARGET: p1 = A...L"
           " (S = SEALED, M = MOBILE, K = KNOWN)\n");
    Print_Bar();
    //--------------------------------------------
    printf(" Success Probability: p1 <- p0\n");
    Print_Bar();
    printf(" p1    =  ");
    For (i,12) printf("  %c  ",i+'A');
    printf("\n");
    For (i,2){
        printf(" p1 <- %s ", ST[i]);
        For (p,12) printf("%5.2f", mST2_CaptureProbMtx[p+'A'][i][0]);
        printf("\n");
    }
    //--------------------------------------------
    Print_Bar();
    printf(" Expected Value: p1 <- p0\n");
    Print_Bar();
    printf(" p1    =  ");
    For (i,12) printf("  %c  ",i+'A');
    printf("\n");
    For (j,2) For (i,3){
        //if (2-i == SEALED && j == SEALED) continue;
        printf("  %s <- %s ", ST[2-i], ST[j]);
        For (p,12)
        printf("%5.1f",
               mST2_ExpValSuccessMtx[p+'A'][2-i][j][0] +
               mST2_ExpValFailureMtx[p+'A'][2-i][j][0]);
        printf("\n");
    }
    //--------------------------------------------
    Print_Bar();
    printf(" Expected Value at Success: p1 <- p0\n");
    Print_Bar();
    printf(" p1    =  ");
    For (i,12) printf("  %c  ",i+'A');
    printf("\n");
    For (j,2) For (i,3){
        //if (2-i == SEALED && j == SEALED) continue;
        printf("  %s <- %s ", ST[2-i], ST[j]);
        For (p,12)
        printf("%5.1f", mST2_ExpValSuccessMtx[p+'A'][2-i][j][0]);
        printf("\n");
    }
    //--------------------------------------------
    Print_Bar();
    printf(" Expected Value at Failure: p1 <- p0\n");
    Print_Bar();
    printf(" p1    =  ");
    For (i,12) printf("  %c  ",i+'A');
    printf("\n");
    For (j,2) For (i,3){
        //if (2-i == SEALED && j == SEALED) continue;
        printf("  %s <- %s ", ST[2-i], ST[j]);
        For (p,12)
        printf("%5.1f", mST2_ExpValFailureMtx[p+'A'][2-i][j][0]);
        printf("\n");
    }
    Print_Bar(); Print_Bar();
    //--------------------------------------------
    //--------------------------------------------
}
//---------------------------------------------------------------------
//---------------------------------------------------------------------
void Logic::ST2_BasicStats(bool disp, int ply){
    
    //--------------------------------
    bool isWhite = !bool(ply%2);
    char A = isWhite ? 'a' : 'A', L = isWhite ? 'l' : 'L', p;
    int status;
    //--------------------------------
    int Freq_Unknown[SPARSE];
    int Freq_SealedStatic[SPARSE];
    int Freq_UnknownDynamic[SPARSE];
    int N_Unknown        = 0;
    int N_Sealed         = 0;
    int N_Mobile         = 0;
    int N_SealedStatic   = 0;
    int N_UnknownDynamic = 0;
    //--------------------------------
    For (j, 12) {
        p = j + A;
        Freq_Unknown[p] =
        Freq_SealedStatic[p] =
        Freq_UnknownDynamic[p] = 0;
        For(f, FILES) For(r, RANKS) {
            if (mGamePos[ply].sq[f][r] == p) {
                status = mGamePosStatus[ply].sq[f][r];
                if (status != KNOWN) {
                    Freq_Unknown[p]++;
                    N_Unknown++;
                    if (status == SEALED) {
                        N_Sealed++;
                        if (p == A || p == L) {
                            Freq_SealedStatic[p]++;
                            N_SealedStatic++;
                        } else {
                            Freq_UnknownDynamic[p]++;
                            N_UnknownDynamic++;
                        }
                    } else {
                        Freq_UnknownDynamic[p]++;
                        N_Mobile++;
                        N_UnknownDynamic++;
                    }
                }
            }
        }
    }
    //--------------------------------
    mST2_N_SealedDynamic = N_Sealed - N_SealedStatic;
    //--------------------------------
    float q;
    For(j, 12) {
        p = j + A;
        if (Freq_Unknown[p] == 0) {
            For(status, 2) mST2_StatusToProb4Piece[p][status] = 0.f;
            continue;
        }
        if (p == A || p == L) {
            mST2_StatusToProb4Piece[p][SEALED] =
                float(Freq_SealedStatic[p]) / float(N_Sealed);
            mST2_StatusToProb4Piece[p][MOBILE] = 0.0f;
        }
        else {
            q = N_UnknownDynamic == 0 ? 0.0f :
               float(Freq_UnknownDynamic[p]) / float(N_UnknownDynamic);
            mST2_StatusToProb4Piece[p][SEALED] = 
                q * float(mST2_N_SealedDynamic) / float(N_Sealed);
            mST2_StatusToProb4Piece[p][MOBILE] = 
                N_Mobile == 0 ? 0.0f : q;
        }    
    }
    //--------------------------------
    if (!disp) return;

    //Comment the lines below (until the end of method)
    //before the submission of this file

    /*
    //--------------------------------Print
    printf(" Unknown Pieces         = %d\n", N_Unknown);
    printf(" Sealed Pieces          = %d\n", N_Sealed);
    printf(" Mobile Pieces          = %d\n", N_Mobile);
    printf(" Sealed Static Pieces   = %d\n", N_SealedStatic);
    printf(" Sealed Dynamic Pieces  = %d\n", mST2_N_SealedDynamic);
    printf(" Unknown Dynamic Pieces = %d\n", N_UnknownDynamic);
    //--------------------------------
    Print_Bar();
    printf(" Unknown:          ");
    For(j, 12) {
        if (Freq_Unknown[j + A] > 0)
            if (Freq_Unknown[j + A] == 1)
                printf("%c ", j + A);
            else printf("%dx%c ", Freq_Unknown[j + A], j + A);
    }
    printf("\n");
    //--------------------------------
    Print_Bar();
    printf(" Sealed Static:    ");
    For(j, 12) {
        if (Freq_SealedStatic[j + A] > 0)
            if (Freq_SealedStatic[j + A] == 1)
                printf("%c ", j + A);
            else printf("%dx%c ", Freq_SealedStatic[j + A], j + A);
    }
    printf("\n");
    //--------------------------------
    Print_Bar();
    printf(" Unknown Dynamic:  ");
    For(j, 12) {
        if (Freq_UnknownDynamic[j + A] > 0)
            if (Freq_UnknownDynamic[j + A] == 1)
                printf("%c ", j + A);
            else printf("%dx%c ", Freq_UnknownDynamic[j + A], j + A);
    }
    printf("\n");
    //--------------------------------
    Print_Bar();
    float sum[2]; sum[0] = sum[1] = 0.f;
    For(j, 12) {
        char p = j + A;
        printf(" P(%c|SEALED) = %4.3f   P(%c|MOBILE) = %4.3f\n",
            p, mST2_StatusToProb4Piece[p][SEALED],
            p, mST2_StatusToProb4Piece[p][MOBILE]);
        For(s, 2) sum[s] += mST2_StatusToProb4Piece[p][s];
    }
    printf(" SUM           %4.3f   SUM           %4.3f\n",
        sum[SEALED], sum[MOBILE]);
    //--------------------------------
    Print_Bar(); Print_Bar();
    //--------------------------------
    */
}
//---------------------------------------------------------------------
//---------------------------------------------------------------------
void Logic::ST2_BuildExpValSubMtx(char s0, char s1,
                                  int playerIsStriker, int ply){
    //s0: status striker, s1: status target
    //----------------------------------
    bool isWhite = !bool(ply%2);
    char playerA   = isWhite ? 'A' : 'a';
    char opponentA = isWhite ? 'a' : 'A';
    //----------------------------------
    For (i,12){
        char p0 = i + playerA;
        mST2_CaptureProbMtx[p0][s1][playerIsStriker] =
        mST2_ExpValSuccessMtx[p0][s0][s1][playerIsStriker] =
        mST2_ExpValFailureMtx[p0][s0][s1][playerIsStriker] = 0.f;
        
        if (playerIsStriker && (i == 0 || i == 11)) continue;
        
        For (j,12){
            
            //----------------------------------
            if (!playerIsStriker && (j == 0 || j == 11)) continue;

            //----------------------------------
            char p1 = j + opponentA;
            
            //----------------------------------
            char finalPiece = playerIsStriker ?
                mST_CaptureMtx[p0][p1] : mST_CaptureMtx[p1][p0];
            
            //----------------------------------
            bool success = playerIsStriker ?
                p0 == finalPiece : p1 == finalPiece;
            
            //----------------------------------
            //----------------------------------
            //----------------------------------
            float P = mST2_StatusToProb4Piece[p1][s1];
            //----------------------------------
            //----------------------------------
            //----------------------------------
            float bef =
                mGamePosST[ply].pieceValue[p0] +
                mGamePosST[ply].pieceValue[p1] +
                mST_PieceStatusValue[p0][s0] +
                mST_PieceStatusValue[p1][s1];
            
            //----------------------------------
            float after =
                mGamePosST[ply].pieceValue[finalPiece] +
                mST_PieceStatusValue[finalPiece][KNOWN];
            
            //----------------------------------
            float ExpValDelta = P * (after-bef);
            //printf("%f\n", ExpValDelta);
            if (success){
                mST2_CaptureProbMtx[p0][s1][playerIsStriker] += P;
                mST2_ExpValSuccessMtx[p0][s0][s1][playerIsStriker] +=
                ExpValDelta;
            }
            else mST2_ExpValFailureMtx[p0][s0][s1][playerIsStriker] +=
                ExpValDelta;
            //----------------------------------
            if (ST2_PRINT == ST2_PRINT_VERBOSE){
                const char ST[3][16] = {"Sealed","Mobile","Known"};
                const char RS[2][16] = {"Failure","Success"};
                float part = P * (after-bef) + .00001;
                if (playerIsStriker)
                    printf(" %s %c strikes %s %c",
                           ST[s0], p0, ST[s1], p1);
                else 
                    printf(" %s %c strikes %s %c",
                           ST[s1], p1, ST[s0], p0);
                printf(" : %3.3f : %7.3f", P + .00001, part);
                if (P > 0.f) printf(" : %s\n", RS[success]);
                else printf(" : N/A\n");
            }
            //----------------------------------
        }
        //----------------------------------
        mST2_ExpValMtx[p0][s0][s1][playerIsStriker] =
            mST2_ExpValSuccessMtx[p0][s0][s1][playerIsStriker] +
            mST2_ExpValFailureMtx[p0][s0][s1][playerIsStriker];
        //----------------------------------
        if (ST2_PRINT == ST2_PRINT_VERBOSE){
            Print_Bar();
            printf(" Probability of Success:    %7.3f\n",
                   mST2_CaptureProbMtx[p0][s1][playerIsStriker]);
            printf(" Expected Value:            %7.3f\n",
                   mST2_ExpValMtx[p0][s0][s1][playerIsStriker]);
            printf(" Expected Value at Success: %7.3f\n",
                   mST2_ExpValSuccessMtx[p0][s0][s1][playerIsStriker]);
            printf(" Expected Value at Failure: %7.3f\n",
                   mST2_ExpValFailureMtx[p0][s0][s1][playerIsStriker]);
            Print_Bar(); Print_Bar();
        }
    }
}
//---------------------------------------------------------------------
//---------------------------------------------------------------------
















//---------------------------------------------------------------------
//---------------------------------------------------------------------
//                     STRATEGO 2 - Find Children
//---------------------------------------------------------------------
//---------------------------------------------------------------------
int Logic::ST2_FindChildren(int ply){
    
    bool isPlayer = bool(ply%2) == bool(mStartPly%2);
    
    mMove[ply].children = 0;
    For (x,FILES) For (y,RANKS){
        char p = mGamePos[ply].sq[x][y];
        if (p == '*') continue;//Empty square
        if (!mIsWhite) p -= 32;//Map ASCII-char lowercase to uppercase
        //----------------------------------
        if (isPlayer){
            if (p == 'C'){Rook(x,y,ply); continue;}
            if (p >= 'B' && p <= 'K') ST_BasicMoves(x,y,ply);
        }
        //----------------------------------
        else {//isOpponent: Rook motion excluded for simplicity
            if (mGamePosStatus[ply].sq[x][y] == MOBILE){
                if (p >= 'B' && p <= 'K') ST_BasicMoves(x,y,ply);
            }
            else {//SEALED
                bool couldBeDynamic = p >= 'A' && p <= 'L' &&
                    mST2_N_SealedDynamic > 0;
                if (couldBeDynamic) ST_BasicMoves(x,y,ply);
            }
        }
        //----------------------------------
    }
    //---------------------------------Move best child to top of list   
    if (CORE_ENGINE == OPTIM && ply < mMaxPly-1){
        OptChildOrder(ply);
    }
    return mMove[ply].children;
}
//---------------------------------------------------------------------
//---------------------------------------------------------------------
//                   STRATEGO 2 - Update Game Pos
//---------------------------------------------------------------------
//---------------------------------------------------------------------
void Logic::ST2_UpdateGamePos(char childIdx, int ply){
    
    
    //Definitions: PLAYER is defined as the side that is expected to
    //make a move at ply = mStartPly + N.
    //OPPONENT is defined as the side that is expected to make a move
    //at ply = mStartPly + 1 + N,
    //where in both cases, N is any positive even number..
    //(i.e. 0, 2, 4, ... etc.)
    
    //By definition in this method, PLAYER can here only be a machine,
    //while OPPONENT can be a machine or a human (does not make any
    //difference for the PLAYER)

    
    //------------------------------Short forms
    int D = ply - 1,
    x0 = mMove[D].child[childIdx][0], y0 = mMove[D].child[childIdx][1],
    x1 = mMove[D].child[childIdx][2], y1 = mMove[D].child[childIdx][3];
    //-----------------------------------------
    mGamePosST[ply].deltaExpVal = mGamePosST[D].deltaExpVal;
    //--------------------------Copy game position
    //mGamePos, mGamePosStatus and mGamePosST are a STRUCTS, so the
    //whole board is copied here, not the pointer:
    mGamePos[ply]       = mGamePos[D];
    mGamePosStatus[ply] = mGamePosStatus[D];
    mGamePosST[ply]     = mGamePosST[D];
    //-----------------------------------------Striker
    char p0 = mGamePos[ply].sq[x0][y0];
    char s0 = mGamePosStatus[ply].sq[x0][y0];
    //-----------------------------------------Target
    char p1 = mGamePos[ply].sq[x1][y1];
    char s1 = mGamePosStatus[ply].sq[x1][y1], new_s1;
    //-----------------------------------------
    bool isStartPlyWhite = !bool(mStartPly%2);
    bool isWhite = !bool(ply%2);
    bool playerIsStriker = isWhite == isStartPlyWhite;
    //-----------------------------------------
    float before, after;
    //-----------------------------------------
    
    
    
    //---------------------------------------------
    //---------------------------------------------
    // Non-Probabilistic Sequence - Imperfect Info
    //---------------------------------------------
    //---------------------------------------------
    if (p1 == '*'){//Move to empty square
        //-------------------------------------Pos
        mGamePos[ply].sq[x0][y0] = '*';
        mGamePos[ply].sq[x1][y1] = p0;
        //-------------------------------------Status
        if (p0 == 'c' || p0 == 'C') new_s1 = KNOWN;
        else new_s1 = s0 != KNOWN ? MOBILE : KNOWN;
        mGamePosStatus[ply].sq[x1][y1] = new_s1;
        //-------------------------------------Delta
        before = mST_PieceStatusValue[p0][s0]     + mRankGrad[y0];
        after  = mST_PieceStatusValue[p0][new_s1] + mRankGrad[y1];
        mGamePosST[ply].deltaExpVal += after - before;
        //-------------------------------------
        return;
        //-------------------------------------
    }
    //-----------------------------------------
    if (s1 == KNOWN){
        //Opponent's piece is known => we know the
        //outcome of a strike no matter who strikes
        //-------------------------------------Pos
        char finalPiece = mST_CaptureMtx[p0][p1];
        mGamePos[ply].sq[x0][y0] = '*';
        mGamePos[ply].sq[x1][y1] = finalPiece;
        //-------------------------------------Status
        new_s1 = s0 != KNOWN ? MOBILE : KNOWN;
        mGamePosStatus[ply].sq[x1][y1] = new_s1;
        //-------------------------------------Delta
        before =
            mGamePosST[ply].pieceValue[p0] +
            mGamePosST[ply].pieceValue[p1] +
            mST_PieceStatusValue[p0][s0] +
            mST_PieceStatusValue[p1][s1] +
            mRankGrad[y0] + mRankGrad[y1];
        after =
            mGamePosST[ply].pieceValue[finalPiece] +
            mST_PieceStatusValue[finalPiece][new_s1] +
            (finalPiece != '*' ? mRankGrad[y1] : 0.f);//Even
        mGamePosST[ply].deltaExpVal += after - before;
        //-------------------------------------
        return;
        //-------------------------------------
    }
    //-----------------------------------------

    
    
    //---------------------------------------------
    //---------------------------------------------
    // Probabilistic Sequence - Imperfect Info
    //---------------------------------------------
    //---------------------------------------------
    

    
    //-------------------------------------Delta
    mGamePosST[ply].deltaExpVal -= mRankGrad[y0];
    mGamePosST[ply].deltaExpVal += playerIsStriker ?
    mST2_ExpValMtx[p0][s0][s1][1] : mST2_ExpValMtx[p1][s1][s0][0];//#1
    //-------------------------------------
    bool strikerWins =
    (isWhite && (mGamePosST[ply].deltaExpVal >= 0)) ||
    (!isWhite && (mGamePosST[ply].deltaExpVal < 0));//#2
    //-------------------------------------Pos
    mGamePos[ply].sq[x0][y0] = '*';
    if (strikerWins) mGamePos[ply].sq[x1][y1] = p0;
    //-------------------------------------Status
    bool playerWins =
    ( playerIsStriker &&  strikerWins) ||
    (!playerIsStriker && !strikerWins);
    mGamePosStatus[ply].sq[x1][y1] = playerWins ? KNOWN : MOBILE;//#3
    //-------------------------------------
    
}
//---------------------------------------------------------------------
//---------------------------------------------------------------------






//---------------------------------------------------------------------
//---------------------------------------------------------------------
//                              REVERSI
//---------------------------------------------------------------------
//---------------------------------------------------------------------
void Logic::InitReversi() {
    mDeltaPly = 5;
    mRandAmplitude = .1;
    const char START[][16] = { "* * * * * * * *",
                               "* * * * * * * *",
                               "* * * * * * * *",
                               "* * * b W * * *",
                               "* * * W b * * *",
                               "* * * * * * * *",
                               "* * * * * * * *",
                               "* * * * * * * *" };
    For(x, FILES) For(y, RANKS)
        mGamePos[0].sq[x][y] = START[RANKS_1-y][2*x];
    FindChildren(0);
    Reversi_GoalFunc(false, true, 0);
}
//---------------------------------------------------------------------
//---------------------------------------------------------------------
int Logic::Reversi_FindChildren(int ply) {
    bool endsWithPlayer = false, opponentFound = false;
    const int d[][2] = {1,0, 1,1, 0,1, -1,1, -1,0, -1,-1, 0,-1, 1,-1};
    int status;
    mMove[ply].children = 0;
    For (x, FILES) For (y, RANKS)
        if (mGamePos[ply].sq[x][y] == '*' &&
            Reversi_IsLegalMove(x, y, !bool(ply%2), ply))
                    SuggestAddChild(x, y, x, y, ply);
    if (mMove[ply].children == 0) return 0;
    //---------------------------------Move best child to top of list   
    if (CORE_ENGINE == OPTIM && ply < mMaxPly - 1)
        OptChildOrder(ply);
    return mMove[ply].children;
}
//---------------------------------------------------------------------
//---------------------------------------------------------------------
void Logic::Reversi_CopyBoard(int fromPly, int toPly) {
    //Update (copy) game board - also used to skip a turn
    mGamePos[toPly] = mGamePos[fromPly];
}
//---------------------------------------------------------------------
//---------------------------------------------------------------------
void Logic::Reversi_UpdateGamePos(char childIdx, int ply) {
    //--------------------------Short forms
    int D = ply - 1, revs = 0,
    x = mMove[D].child[childIdx][2], y = mMove[D].child[childIdx][3];
    const int d[][2] = {1,0, 1,1, 0,1, -1,1, -1,0, -1,-1, 0,-1, 1,-1};
    char plyr = bool(ply%2) ? 'W' : 'b', rev[SPARSE][2], offset;
    //--------------------------Copy previous board
    Reversi_CopyBoard(D, ply);
    //--------------------------Add piece to copied board
    mGamePos[ply].sq[x][y] = plyr;
    //--------------------------Reverse effected pieces
    For(dir, 8) {
        for (int i = x + d[dir][0], j = y + d[dir][1];
            Reversi_IsBound(i, j);
            i += d[dir][0], j += d[dir][1]) {
            offset = mGamePos[ply].sq[i][j];
            if (offset == '*') break;
            else if (offset == plyr) {
                For(k, revs)
                    mGamePos[ply].sq[rev[k][0]][rev[k][1]] = plyr;
                break;
            }
            rev[revs][0] = i;
            rev[revs][1] = j;
            revs++;
        }
        revs = 0;
    }
}
//---------------------------------------------------------------------
//---------------------------------------------------------------------
bool Logic::Reversi_IsMoveOK_ManualPlay(char move[4], int ply) {
    bool moveFound = false;
    //---------------------------------------
    For(i, mMove[ply].children) {
        if (mMove[ply].child[i][0] == move[0] &&
            mMove[ply].child[i][1] == move[1] &&
            mMove[ply].child[i][2] == move[2] &&
            mMove[ply].child[i][3] == move[3]) {
            moveFound = true;
            mMove[ply].selectedChild = i;
            break;
        }
    }
    return moveFound;
}
//---------------------------------------------------------------------
//---------------------------------------------------------------------
bool Logic::Reversi_IsLegalMove(int x, int y, bool isWhite, int ply) {
    char plyr = isWhite ? 'W' : 'b', offset;
    const int d[][2] = {1,0, 1,1, 0,1, -1,1, -1,0, -1,-1, 0,-1, 1,-1};
    bool endsWithPlayer = false, opponentFound = false;
    For(dir, 8) {
        for (int i = x + d[dir][0], j = y + d[dir][1];
            Reversi_IsBound(i, j);
            i += d[dir][0], j += d[dir][1]) {
            offset = mGamePos[ply].sq[i][j];
            if (offset == '*') break;
            else if (endsWithPlayer = offset == plyr) break;
            opponentFound = true;
        }
        if (opponentFound && endsWithPlayer)
            return true;
        opponentFound = endsWithPlayer = false;
    }
    return false;
}
//---------------------------------------------------------------------
//---------------------------------------------------------------------
bool Logic::Reversi_IsFutureMove(int x, int y, bool isWhite, int ply) {
    char plyr = isWhite ? 'W' : 'b';
    const int d[][2] = {1,0, 1,1, 0,1, -1,1, -1,0, -1,-1, 0,-1, 1,-1};
    For(dir, 8) {
        int i = x + d[dir][0], j = y + d[dir][1];
        if (Reversi_IsBound(i, j) && mGamePos[ply].sq[i][j] == plyr) {
            return true;
        }
    }
    return false;
}
//---------------------------------------------------------------------
//---------------------------------------------------------------------
bool Logic::Reversi_IsBound(int x, int y) {
    return x >= 0 && x < FILES && y >= 0 && y < RANKS;
}
//---------------------------------------------------------------------
//---------------------------------------------------------------------
bool Logic::Reversi_IsCorner(int x, int y) {
    return (x == 0 || x == FILES_1) && (y == 0 || y == RANKS_1);
}
//---------------------------------------------------------------------
//---------------------------------------------------------------------
float Logic::Reversi_GoalFunc(bool incrN, bool randOn, int ply) {
    int w[] = { P, M, FM, CC, S };
    char maxPlyr           = 'W', minPlyr           = 'b', s;
    int  maxPlyrDiscs      = 0,   minPlyrDiscs      = 0,
         maxPlyrMobility   = 0,   minPlyrMobility   = 0,
         maxPlyrFutureMob  = 0,   minPlyrFutureMob  = 0,
         maxPlyrCornerCtrl = 0,   minPlyrCornerCtrl = 0,
         maxPlyrStability  = 0,   minPlyrStability  = 0;
    int stabilityMatrix[8][8] = { {  4, -3,  2,  2,  2,  2, -3,  4 },
                                  { -3, -4, -1, -1, -1, -1, -4, -3 },
                                  {  2, -1,  1,  0,  0,  1, -1,  2 },
                                  {  2, -1,  0,  1,  1,  0, -1,  2 },
                                  {  2, -1,  0,  1,  1,  0, -1,  2 },
                                  {  2, -1,  1,  0,  0,  1, -1,  2 },
                                  { -3, -4, -1, -1, -1, -1, -4, -3 },
                                  {  4, -3,  2,  2,  2,  2, -3,  4 } };
    float v, parity, mobility, futureMob, cornerCtrl, stability;

    For(x, FILES) For(y, RANKS) {
        s = mGamePos[ply].sq[x][y];
        if (s == maxPlyr) {
            maxPlyrDiscs++;
            maxPlyrStability += stabilityMatrix[x][y];
            if (Reversi_IsCorner(x, y)) maxPlyrCornerCtrl++;
        }
        else if (s == minPlyr) {
            minPlyrDiscs++;
            minPlyrStability += stabilityMatrix[x][y];
            if (Reversi_IsCorner(x, y)) minPlyrCornerCtrl++;
        }
        else { // <-- empty
            if (Reversi_IsLegalMove(x, y, true, ply))
                maxPlyrMobility++;
            else if (Reversi_IsFutureMove(x, y, true, ply))
                maxPlyrFutureMob++;
            if (Reversi_IsLegalMove(x, y, false, ply))
                minPlyrMobility++;
            else if (Reversi_IsFutureMove(x, y, false, ply))
                minPlyrFutureMob++;
        }
    }
    if ((maxPlyrDiscs + minPlyrDiscs) != 0) {
        parity = w[0] * (maxPlyrDiscs - minPlyrDiscs) /
                        (maxPlyrDiscs + minPlyrDiscs);
    } else { parity = 0; }
    if ((maxPlyrMobility + minPlyrMobility) != 0) {
        mobility = w[1] * (maxPlyrMobility - minPlyrMobility) /
                          (maxPlyrMobility + minPlyrMobility);
    } else { mobility = 0; }
    if ((maxPlyrFutureMob + minPlyrFutureMob) != 0) {
        futureMob = w[2] * (maxPlyrFutureMob - minPlyrFutureMob) /
                           (maxPlyrFutureMob + minPlyrFutureMob);
    } else { futureMob = 0; }
    if ((maxPlyrCornerCtrl + minPlyrCornerCtrl) != 0) {
        cornerCtrl = w[3] * (maxPlyrCornerCtrl - minPlyrCornerCtrl) /
                            (maxPlyrCornerCtrl + minPlyrCornerCtrl);
    } else { cornerCtrl = 0; }
    if ((maxPlyrStability + minPlyrStability) != 0) {
        stability = w[4] * (maxPlyrStability - minPlyrStability) /
                           (maxPlyrStability + minPlyrStability);
    } else { stability = 0; };
    v = parity + mobility + futureMob + cornerCtrl + stability;
    if (incrN) mN++;
    return (randOn ? v + mRandAmplitude * Rand() : v);
}
//---------------------------------------------------------------------
//---------------------------------------------------------------------
int Logic::Reversi_DiscCount(bool isWhite, int ply) {
    int count = 0;
    char w = isWhite ? 'W' : 'b';
    For(x, FILES) For(y, RANKS)
        if (mGamePos[ply].sq[x][y] == w)
            count++;
    return count;
}









