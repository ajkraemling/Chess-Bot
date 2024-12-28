#include <iostream>
#include <unordered_map>
#include <tuple>
#include <vector>
using namespace std;

enum pieceType { KING, QUEEN, ROOK, BISHOP, KNIGHT, PAWN };
enum pieceSide { WHITE, BLACK };

class Piece {
protected:
    unordered_map<pair<char, int>, Piece*>* board;
    pair<char, int> location;
    int value;
    pieceType type;
    pieceSide color;

public:
    Piece(unordered_map<pair<char, int>, Piece*>& board, int value, pair<char, int> location, pieceType type, pieceSide color)
        : board(&board), value(value), location(location), type(type), color(color) {
        (*board)[location] = this; // Add piece to the board
    }

    virtual ~Piece() {
        // Remove piece from the board if the destructor is called
        if (board && board->count(location)) {
            board->erase(location);
        }
    }

    // Pure virtual method to calculate valid moves
    virtual vector<pair<char, int>> validMoves(const bool danger[8][8]) = 0;

    // Move piece to a new location
    virtual void makeMove(pair<char, int> move) {
        if (board) {
            (*board)[move] = (*board)[location];
            (*board).erase(location);
            location = move;
        }
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
    pair<char, int> getLocation() const {
        return location;
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
            return piece->getType() == ROOK && !piece->hasMoved();
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
    King(unordered_map<pair<char, int>, Piece*>& board, pieceSide color) 
        : Piece(board, 0, color == WHITE ? make_pair('d', 1) : make_pair('e', 8), KING, color) {}
    
    // Takes in board state, all 'dangerous' squares on the board
    // Returns all possible spaces piece can go to (all diagonals, horizontals, checks for pieces in the way, for takes, for checks, castles, etc)
    vector<pair<char, int>> validMoves(const bool danger[8][8]) override {
        vector<pair<char, int>> moves;
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

    void makeMove(pair<char, int> move) override{
        int curr_rank = location.second;

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
        board[move] = board[location];
        board[location] = nullptr;

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