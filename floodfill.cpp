/*  Author: Patrick Gelvin
    Date: April 1st, 2021
    Usage: ./floodfill
        Then follow on screen instructions
*/

#include <iostream>
#include <stdlib.h>
#include <ctime>
#include <queue>
#include <limits>

// Constants
const char emptyChar = ' ';
const char solidChar = '#';
const char mineTileChar = '-';
const char mineSelectedChar = '^';
const char mineFlaggedChar = '!';
const std::pair<int, int> tileOffsetsOrthog[4] = {{1, 0}, {0, 1}, {-1, 0}, {0, -1}};
const std::pair<int, int> tileOffsetsExtend[8] = {{-1, -1}, {0, -1}, {1, -1}, {-1, 0}, {1, 0}, {-1, 1}, {0, 1}, {1, 1}};

// Utility struct to keep width + height info with array
struct Array2D {
    int width, height;
    char** array;

    Array2D(int w, int h, char initValue = 0) : width(w), height(h) {
        // Array initialization
        array = new char*[width];
        for (int i = 0; i < width; ++i) {
            array[i] = new char[height];
            for (int j = 0; j < height; ++j) {
                array[i][j] = initValue;
            }
        }
    }

    // Overload [] operator for ease of access
    char* operator[](int index) { return array[index]; }

    // Utility function to print contents of array to output stream
    // << overload is *technically* not a member function, hence the friend
    friend std::ostream& operator<<(std::ostream& os, const Array2D& arr) {
        // Print border around array
        os << "┌";
        for (int i = 0; i < arr.width; ++i) {
            os << "─";
        }
        os << "┐" << std::endl;

        for (int j = 0; j < arr.height; ++j) {
            os << "│";
            for (int i = 0; i < arr.width; ++i) {
                os << arr.array[i][j];
            }
            os << "│" << std::endl;
        }

        os << "└";
        for (int i = 0; i < arr.width; ++i) {
            os << "─";
        }
        os << "┘" << std::endl;

        return os;
    }

    // Helper function to check if index is in bounds
    bool pointValid(int x, int y) {
        return (x >= 0 &&
                x < width &&
                y >= 0 &&
                y < height);
    }

    // Destructor to clean up memory
    ~Array2D() {
        for (int i = 0; i < width; ++i) {
            delete[] array[i];
        }
        delete[] array;
    }

};

// Forward declaration of functions
int getInt(const char* const);
char getChar(const char* const);
void floodFill(Array2D&, char, int, int);
void mineLoop(Array2D&, int);
void mineFill(Array2D&, Array2D&, int, int, int&);
void mineReveal(Array2D&, Array2D&);

int main() {
    // Step 1: Ask user for width and height
    int width, height;
    do 
        width = getInt("Please enter width: ");
    while (width <= 0); // Make these do while?
    
    do
        height = getInt("Pleae enter height: ");
    while (height <= 0);
    
    int area = width * height;

    // Initialize array
    Array2D array = Array2D(width, height, emptyChar);

    // Step 2: Ask user for percent impassible
    int percentImpassible;
    do {
        percentImpassible = getInt("Please enter percent impassible: ");
        if (percentImpassible > 40 && percentImpassible < 100) {
            // Verify user wants this percent
            char verify = getChar("Warning! Values greater than 40% could cause poor results!\n"
                                    "Please enter uppercase Y to confirm: ");
            if (verify != 'Y')
                percentImpassible = -1;
        }
    } while (percentImpassible < 0 || percentImpassible > 99);

    // Step 3: Fill array with impassible tiles
    int numImpassible = area * (percentImpassible / 100.0);

    std::srand(std::time(NULL));
    for (int n = 0; n < numImpassible; ++n) {
        while (true) {
            // Step 3a: Select a random row and column
            int i = rand() % array.width;
            int j = rand() % array.height;

            // Step 3b: Make sure tile isn't *already* impassible
            if (array[i][j] == solidChar)
                continue;
            
            // Step 3c: Make tile impassible and exit while loop
            array[i][j] = solidChar;
            break;
        }
    }

    // Ask user if they want to play minesweeper with the generated board
    char game = getChar("Want to play MineSweeper with this board?\n"
                        "Enter uppercase Y to play or anything else to flood fill: ");
    
    if (game == 'Y') {
        mineLoop(array, numImpassible);
        return 0;
    }

    // Print grid to user
    std::cout << array;

    // Step 4: Get drop coords
    int dropX, dropY;
    do {
        std::cout << "Please select a blank tile to start the fill from\n"
                        "(Coordinates are 0 indexed)\n";
        do
            dropX = getInt("Please enter X coordinate: ");
        while (dropX < 0 || dropX >= array.width);

        do
            dropY = getInt("Please enter Y coordinate: ");
        while (dropY < 0 || dropY >= array.height);
    } while (array[dropX][dropY] == solidChar);

    // Step 5: Get fill character
    char fillChar = getChar("Please enter character to fill with: ");

    // Step 6: Actually do the flood fill
    floodFill(array, fillChar, dropX, dropY);

    // Step 7: Print result
    std::cout << array;

    return 0;
}

int getInt(const char* const prompt) {
    int val;
    while(true) {
        std::cout << prompt;
        std::cin >> val;
        if (std::cin.fail()) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // I hate how verbose this is too.
            continue;
        } else break;
    }

    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Don't trust the user, ever

    return val;
}

char getChar(const char* const prompt) {
    char ch;
    while(true) {
        std::cout << prompt;
        std::cin >> ch;
        if (std::cin.fail()) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        } else break;
    }

    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    return ch;
}

void floodFill(Array2D& array, char fillChar, int startX, int startY) {
    // Perform a breadth-first search to flood fill
    std::queue<std::pair<int, int>> Q; // Pair is the X and Y, respectively

    // Step 1: Fill drop point (using filled as discovered)
    array[startX][startY] = fillChar;

    // Step 2: Enqueue start point
    Q.emplace(startX, startY);

    // Step 3: Run until no more tiles to fill
    while (!Q.empty()) {
        
        auto [x, y] = Q.front();
        Q.pop();

        // Step 3a: Check neighboring tiles
        for (auto& [dx, dy]: tileOffsetsOrthog) {
            // Calculate test index
            int tx = x + dx;
            int ty = y + dy;

            // Make sure index is in bounds and valid move (using conditional short circuiting)
            if (array.pointValid(tx, ty) && array[tx][ty] != fillChar && array[tx][ty] != solidChar) {
                // Fill point and add to queue

                std::cout << array << "\n";
                
                array[tx][ty] = fillChar;
                Q.emplace(tx, ty);
            }
        }
    }
}

void mineLoop(Array2D& mines, int numMines) {
    // Game preperation
    // Find number of mines nearby in each empty cell
    Array2D board = Array2D(mines.width, mines.height, mineTileChar);
    int numUnrevealed = mines.width * mines.height;
    int numFlagged = 0;
    for (int i = 0; i < mines.width; ++i) {
        for (int j = 0; j < mines.height; ++j) {
            // Skip if cell is mine
            if (mines[i][j] == solidChar)
                continue;
            
            mines[i][j] = '0';
            
            // Check neighboring tiles
            for (auto& [dx, dy]: tileOffsetsExtend) {
                int tx = i + dx;
                int ty = j + dy;

                if (mines.pointValid(tx, ty) && mines[tx][ty] == solidChar) {
                    ++mines[i][j]; // Thank god ascii numbers are next to each other
                                   // and this works for single digit numbers
                }
            }
        }
    }
    bool running = true;

    // Game loop
    // Each round has several phases
    // Phase 1: Present current board to user
    // Phase 2: Get input from user (coords to reveal/flag)
    // Phase 3: Check for mine (game over)
    // Phase 4: Check for zero mines nearby (flood reveal)
    while (running) {
        // Phase 1
        std::cout << "\n" << "Mines left: " << numMines - numFlagged << "\n" << board;

        // Phase 2
        int x, y;
        x = getInt("Please enter X coord (0 indexed): ");
        y = getInt("Please enter Y coord (0 indexed): ");
        // Verify tile is valid
        if (!board.pointValid(x, y) || ( board[x][y] != mineTileChar && board[x][y] != mineFlaggedChar ))
            continue;
        
        // Confirm tile
        char c, oldChar;
        oldChar = board[x][y];
        board[x][y] = mineSelectedChar; // Reprint board with selected cell a different character
                                        // So user can verify they typed the right numbers
        std::cout << "\n" << "Mines left: " << numMines - numFlagged << "\n" << board;
        c = getChar("What do you want to do: [F]lag/unflag   [R]eveal   [C]ancel: ");
        switch(c) {
            case 'F':
                // Flag/Unflag (Only allow on unrevealed/flagged tiles)
                if (oldChar == mineTileChar) {
                    board[x][y] = mineFlaggedChar;
                    ++numFlagged;
                } else if (oldChar == mineFlaggedChar) {
                    board[x][y] = mineTileChar;
                    --numFlagged;
                }
                break;
            case 'R':
                // Reveal tile (make sure tile is not flagged or already revealed)
                if (oldChar == mineFlaggedChar) {
                    std::cout << "Tile is flagged!\n";
                    board[x][y] = oldChar;
                } else {
                    if (mines[x][y] == solidChar) {
                        // Game over
                        mineReveal(mines, board);
                        std::cout << "Game Over!\n" << board;
                        running = false;
                    } else if (mines[x][y] == '0') {
                        // Flood fill
                        mineFill(mines, board, x, y, numUnrevealed);
                    } else {
                        // Reveal number
                        board[x][y] = mines[x][y];
                        --numUnrevealed;
                    }
                }
                break;
            default:
                board[x][y] = oldChar;
                break;
        }

        // Check for win condition
        if (numUnrevealed == numMines) {
            mineReveal(mines, board);
            std::cout << "You Win!\n" << board;
            running = false;
        }
    }
}

void mineFill(Array2D& mines, Array2D& board, int startX, int startY, int& numUnrevealed) {
    // Breadth-first flood fill algorithm modified to reveal tiles, expanding on blank but still revealing numbers
    // For comment explanation, see floodFill()
    std::queue<std::pair<int, int>> Q;
    board[startX][startY] = emptyChar;
    --numUnrevealed;
    Q.emplace(startX, startY);
    while (!Q.empty()) {
        auto [x, y] = Q.front();
        Q.pop();

        for(auto& [dx, dy]: tileOffsetsExtend) {
            int tx = x + dx;
            int ty = y + dy;

            if (mines.pointValid(tx, ty) && board[tx][ty] == mineTileChar && mines[tx][ty] != solidChar) {
                --numUnrevealed;
                if (mines[tx][ty] == '0') {
                    board[tx][ty] = emptyChar;
                    Q.emplace(tx, ty);
                } else {
                    board[tx][ty] = mines[tx][ty];
                }
            }
        }
    }
}

void mineReveal(Array2D& mines, Array2D& board) {
    for (int x = 0; x < mines.width; ++x) {
        for (int y = 0; y < mines.height; ++y) {
            if (mines[x][y] == solidChar) {
                board[x][y] = solidChar;
            }
        }
    }
}
