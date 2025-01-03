#include <iostream>
#include <unordered_map>
#include <tuple>
#include <vector>
using namespace std;

enum pieceType { KING, QUEEN, ROOK, BISHOP, KNIGHT, PAWN };
enum pieceSide { WHITE, BLACK };

class Piece {
protected:
    unordered_map<pair<char, char>, Piece*>* board;
    pair<char, char> location;
    int value;
    pieceType type;
    pieceSide color;
    Piece* king;

    bool checkPosition(vector<pair<char, char>>& moves, char file, int rank) {
        auto pos = make_pair(file, rank);
        if (board->count(pos) == 0) {
            moves.push_back(pos);
            return true;
        } else if ((*board)[pos]->getColor() != color) {
            moves.push_back(pos);
            return true;
        }
        return false;
    }

    void moveHelper(pair<char, char> move) {
        if (board) {
            if (board->count(move)) (*board)[move]->setLocation(make_pair('z', 'z'));
            (*board)[move] = (*board)[location];
            (*board).erase(location);
            location = move;
        }
    }

public:
    Piece(unordered_map<pair<char, char>, Piece*>& board, Piece& king, int value, pair<char, char> location, pieceType type, pieceSide color)
        : board(&board), king(&king), value(value), location(location), type(type), color(color) {
        (*board)[location] = this; // Add piece to the board
    }

    virtual ~Piece() {
        // Remove piece from the board if the destructor is called
        if (board && board->count(location)) {
            board->erase(location);
        }
    }

    // Pure virtual method to calculate valid moves
    virtual vector<pair<char, char>> validMoves(const bool danger[8][8]) = 0;

    // Move piece to a new location
    virtual void makeMove(pair<char, char> move) {
        moveHelper(move);
    }

    // Get the type of the piece
    virtual pieceType getType() const {
        return type;
    }

    // Get the color of the piece
    virtual pieceSide getColor() const {
        return color;
    }

    // Get the current location of the piece
    pair<char, char> getLocation() const {
        return location;
    }

    void setLocation(pair<char, char> loc) const {
        location = loc; 
    }
};

class King: public Piece {
private:
    bool has_moved = false;
    bool can_castle = true; 
    bool in_check = false;

    bool checkRook(char file, int rank) {
        auto rook_pos = make_pair(file, rank);
        if (board->count(rook_pos)) {
            Piece* piece = (*board)[rook_pos];
            return piece->getType() == ROOK && !static_cast<Rook*>piece->hasMoved();
        }
        return false;
    }

    // Determines if castling is possible on both sides
    pair<bool, bool> canCastle(const bool danger[8][8]) {
        if (!has_moved && !in_check) {
            int rank = location.second;

            // Check that the Castles exist in locations (sanity check, also for unique unequal start positions)
            bool left = checkRook('a', rank);
            bool right = checkRook('h', rank);
            
            // LEFT
            if (left) {
                if (danger['c' - 'a'][rank - 1] || danger['d' - 'a'][rank - 1]) left = false;
                for (char file = 'b'; file <= 'd'; file++) {
                    if (board->count(make_pair(file, rank))) {
                        left = false;
                        break;
                    }
                }
            }
            // RIGHT
            if (right) {
                for (char file = 'f'; file <= 'g'; file++) {
                    if (board->count(make_pair(file, rank)) || danger[file - 'a'][rank - 1]) {
                        right = false;
                        break;
                    }
                }
            }
            can_castle = left || right;
            return {left, right};
        }
        return {false, false};
    }


public: 
    King(unordered_map<pair<char, char>, Piece*>& board, pieceSide color) 
        : Piece(board, this, 0, color == WHITE ? make_pair('d', 1) : make_pair('e', 8), KING, color) {}

    King(unordered_map<pair<char, char>, Piece*>& board, pieceSide color, pair<char, char> location) 
        : Piece(board, this, 0, location, KING, color) {}
    
    // Takes in board state, all 'dangerous' squares on the board
    // Returns all possible spaces piece can go to (all diagonals, horizontals, checks for pieces in the way, for takes, for checks, castles, etc)
    vector<pair<char, char>> validMoves(const bool danger[8][8]) override {
        vector<pair<char, char>> moves;
        char curr_file = location.first;
        char curr_rank = location.second;

        const int directions[8][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {-1, -1}, {1, -1}, {-1, 1}};
        
        for (const auto& dir : directions) {
            char file = curr_file + dir[0];
            int rank = curr_rank + dir[1];
            if (file >= 'a' && file <= 'h' && rank >= 1 && rank <= 8) {
                auto pos = make_pair(file, rank);
                // Check if move is valid (not in danger, not blocked by same-side piece)
                if (!danger[file - 'a'][rank - 1] && (!board->count(pos) ||
                    (*board)[pos]->getColor() != color)) {
                    moves.push_back(file, rank);
                }
            }
        }

        auto [left, right] = canCastle(danger);
        if (left) moves.emplace_back('c', curr_rank);
        if (right) moves.emplace_back('g', curr_rank);

        return moves;
    }

    void makeMove(pair<char, char> move) override{
        char curr_rank = location.second;

        if (!has_moved && can_castle)  {
            // Handle rook movement if castling
            if (move.first == 'c') {
                (*board)[make_pair('d', curr_rank)] = (*board)[make_pair('a', curr_rank)];
                board->erase(make_pair('a', curr_rank));
            } else if (move.first == 'g') {
                (*board)[make_pair('f', curr_rank)] = (*board)[make_pair('h', curr_rank)];
                board->erase(make_pair('h', curr_rank));
            }
        }

        // Perform move
        moveHelper(move);

        has_moved = true; 
        can_castle = false;
    }

    void setInCheck(const bool danger[8][8]) {
        in_check = danger[location.first - 'a'][location.second - 1];
    }

    bool getInCheck() {
        return in_check;
    }
};


class Rook: public Piece {
private:
    bool has_moved = false;

public: 
    Rook(unordered_map<pair<char, char>, Piece*>& board, Piece& king, pieceSide color, pair<char, char> location) 
        : Piece(board, king, 5, location, ROOK, color) {}
    
    vector<pair<char, char>> validMoves() override {
        vector<pair<char, char>> moves;
        char curr_file = location.first;
        char curr_rank = location.second;

        // Check moves to right
        for (char file = curr_file + 1; file <= 'h'; file++) {
            if (checkPosition(moves, file, curr_rank)) break;
        }
        // Check moves to left
        for (char file = curr_file-1; file >= 'a'; file--) {
            if (checkPosition(moves, file, curr_rank)) break;
        }
        // Check moves up
        for (int rank = curr_rank+1; rank <= 8; rank++) {
            if (checkPosition(moves, curr_file, rank)) break;
        }
        // Check moves down
        for (int rank = curr_rank-1; rank >= 1; rank--) {
            if (checkPosition(moves, curr_file, rank)) break;
        }

        return moves;
    }

    void makeMove(pair<char, char> move) override{
        moveHelper(move);

        if (!has_moved) has_moved = true; 
    }

    bool hasMoved() {
        return has_moved;
    }
};


class Bishop: public Piece {
public: 
    Bishop(unordered_map<pair<char, char>, Piece*>& board, Piece& king, pieceSide color, pair<char, char> location) 
        : Piece(board, king, 3, location, BISHOP, color) {}
    
    vector<pair<char, char>> validMoves() override {
        vector<pair<char, char>> moves;
        char curr_file = location.first;
        char curr_rank = location.second;

        // Check moves up right
        for (int i = 1; (curr_file+i <= 'h') && (curr_rank+i <= 8); i++) {
            if (checkPosition(moves, curr_file+i, curr_rank+i)) break;
        }
        // Check moves up left
        for (int i = 1; (curr_file-i >= 'a') && (curr_rank+i <= 8); i++) {
            if (checkPosition(moves, curr_file-i, curr_rank+i)) break;
        }
        // Check moves down right
        for (int i = 1; (curr_file+i <= 'h') && (curr_rank-i >= 1); i++) {
            if (checkPosition(moves, curr_file+i, curr_rank-i)) break;
        }
        // Check moves down left
        for (int i = 1; (curr_file-i >= 'a') && (curr_rank-i >= 1); i++) {
            if (checkPosition(moves, curr_file-i, curr_rank-i)) break;
        }

        return moves;
    }

    void makeMove(pair<char, char> move) override{
        moveHelper(move);

        if (!has_moved) has_moved = true; 
    }
};


class Pawn: public Piece {
private:
    bool has_moved = false;
    pair<char, char> en_passant;
    Piece& en_pawn;
    char direction;

public: 
    Pawn(unordered_map<pair<char, char>, Piece*>& board, Piece& king, pieceSide color, pair<char, char> location) 
        : Piece(board, king, 1, location, PAWN, color) {
            if (color == BLACK) direction = 0;
            else direction = 2;
        }
    
    vector<pair<char, char>> validMoves() override {
        vector<pair<char, char>> moves;
        char curr_file = location.first;
        char curr_rank = location.second;
        int p =(int)direction-1;

        if (checkPosition(moves, curr_file, curr_rank + p)) {
            // Check for double move
            if (!has_moved) {
                pair<char, char> check = make_pair(curr_file, curr_rank + 2*p);
                if (checkPosition(moves, check.first, check.second)) {
                    moves.push_back(check);
                }
            }
        }

        pair<char, char> left = make_pair(move.first-1, move.second+p);
        pair<char, char> right = make_pair(move.first+1, move.second+p);
        if (board->count(left) != 0 && (*board)[left]->getColor() != color) moves.push_back(left);
        if (board->count(right) != 0 && (*board)[right]->getColor() != color) moves.push_back(right);

        return moves;
    }

    // Needs to include triggering other pawns en passant
    void makeMove(pair<char, char> move) override{
        if (en_passant.second != -1 && move == en_passant) {
            en_passant.first = 'z';
            en_passant.second = -1;
            en_pawn->setLocation(make_pair('z', 'z'));
        }

        if ((int)location.second-(int)move.second == 2) {
            pair<char, char> left = make_pair(move.first-1, move.second);
            pair<char, char> right = make_pair(move.first+1, move.second);
            if (board->count(left) != 0 && (*board)[left]->getType() == PAWN && (*board)[left]->getColor() != color) (*board)[pos]->setEnPassant(make_pair(move.first, move.second-(-1*((int)direction-1))), this);
            if (board->count(right) != 0 && (*board)[right]->getType() == PAWN && (*board)[right]->getColor() != color) (*board)[pos]->setEnPassant(make_pair(move.first, move.second-(-1*((int)direction-1))), this);
        }

        // Make move
        moveHelper(move);
        if (!has_moved) has_moved = true; 
    }

    void setEnPassant(pair<char, char> spot, Piece& pawn) {
        en_pawn = pawn;
        en_passant = spot;
    }
};


class Knight: public Piece {
public: 
    Knight(unordered_map<pair<char, char>, Piece*>& board, Piece& king, pieceSide color, pair<char, char> location) 
        : Piece(board, king, 3, location, KNIGHT, color) {}

    vector<pair<char, char>> validMoves(const bool danger[8][8]) override {
        vector<pair<char, char>> moves;
        char curr_file = location.first;
        char curr_rank = location.second;

        const int directions[8][2] = {{2, -1}, {2, 1}, {1, 2}, {1, -2}, {-1, 2}, {-1, -2}, {-2, -1}, {-2, 1}};
        
        for (const auto& dir : directions) {
            char file = curr_file + dir[0];
            int rank = curr_rank + dir[1];
            if (file >= 'a' && file <= 'h' && rank >= 1 && rank <= 8) {
                auto pos = make_pair(file, rank);
                // Check if move is valid (not in danger, not blocked by same-side piece)
                if (!danger[file - 'a'][rank - 1] && 
                    (!board->count(pos) ||
                    (*board)[pos]->getColor() != color)) {
                    moves.push_back(file, rank);
                }
            }
        }

        return moves;
    }
};

class Queen: public Piece {
public: 
    QUEEN(unordered_map<pair<char, char>, Piece*>& board, Piece& king, pieceSide color, pair<char, char> location) 
        : Piece(board, king, 9, location, QUEEN, color) {}
    
    vector<pair<char, char>> validMoves() override {
        vector<pair<char, char>> moves;
        char curr_file = location.first;
        char curr_rank = location.second;

        // Horizontals
        // Check moves to right
        for (char file = curr_file + 1; file <= 'h'; file++) {
            if (checkPosition(moves, file, curr_rank)) break;
        }
        // Check moves to left
        for (char file = curr_file-1; file >= 'a'; file--) {
            if (checkPosition(moves, file, curr_rank)) break;
        }
        // Check moves up
        for (int rank = curr_rank+1; rank <= 8; rank++) {
            if (checkPosition(moves, curr_file, rank)) break;
        }
        // Check moves down
        for (int rank = curr_rank-1; rank >= 1; rank--) {
            if (checkPosition(moves, curr_file, rank)) break;
        }

        // Diagonals
        // Check moves up right
        for (int i = 1; (curr_file+i <= 'h') && (curr_rank+i <= 8); i++) {
            if (checkPosition(moves, curr_file+i, curr_rank+i)) break;
        }
        // Check moves up left
        for (int i = 1; (curr_file-i >= 'a') && (curr_rank+i <= 8); i++) {
            if (checkPosition(moves, curr_file-i, curr_rank+i)) break;
        }
        // Check moves down right
        for (int i = 1; (curr_file+i <= 'h') && (curr_rank-i >= 1); i++) {
            if (checkPosition(moves, curr_file+i, curr_rank-i)) break;
        }
        // Check moves down left
        for (int i = 1; (curr_file-i >= 'a') && (curr_rank-i >= 1); i++) {
            if (checkPosition(moves, curr_file-i, curr_rank-i)) break;
        }

        return moves;
    }
};