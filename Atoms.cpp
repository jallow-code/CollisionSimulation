/*
 * Author: Abdoulie Jallow <jallow.jku@gmail.com>
 * Last modified: 2025/04/01
 */
#include <iostream>
#include <fstream>
#include <sstream>
#include <random>
#include <cmath>
#include <thread>
#include <chrono>
#include "Drawing.h"

using namespace std;
using namespace compsys;

const int W = 640;
const int H = 480;
const int S = 40;      // delay in milliseconds
const int F = 200;     // number of update iterations
const int DEFAULT_N = 10;  // default number of atoms for random generation
const double PI = 3.14159265358979323846;


// Random generation parameters
const double R0 = 10.0;   // minimum radius
const double R1 = 30.0;   // maximum radius
const double V0 = 1.0;    // minimum speed
const double V1 = 5.0;    // maximum speed

// Global random engine (seeded in init)
default_random_engine rng;

struct Atom {
    int color;
    double r;   // radius
    double x, y;  // center position
    double vx, vy; // velocity components
};

//
// number: Determines the number of atoms.
// If no file is given (argc==1), returns DEFAULT_N.
// If a file is provided (argc==2), reads the first number from the file.
//
int number(int argc, const char* argv[]) {
    int n = 0;
    if (argc == 1) {
        n = DEFAULT_N;
    }
    else if (argc == 2) {
        ifstream infile(argv[1]);
        if (!infile) {
            cerr << "Error: Cannot open file " << argv[1] << endl;
            exit(1);
        }
        infile >> n;
        if (!infile || n <= 0) {
            cerr << "Error: Invalid number of atoms in file" << endl;
            exit(1);
        }
    }
    // Print the number of atoms
    cout << n << endl;
    return n;
}