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

//
// init: Initializes the atoms array.
// For random initialization, it generates atoms with random radius,
// position (fully contained in the window and not overlapping with already placed atoms),
// speed and direction, and a random color.
// For file input, it reads the atom values from the given file.
//
void init(int n, Atom atoms[], int argc, const char* argv[]) {
    if (argc == 1) {
        // Seed random generator nondeterministically
        random_device rand_dev;
        rng.seed(rand_dev());

        uniform_real_distribution<double> radius_dist(R0, R1);
        uniform_real_distribution<double> speed_dist(V0, V1);
        uniform_real_distribution<double> angle_dist(0, 2 * PI);
        // For color: generate an RGB color in the range 0x000000 to 0xFFFFFF
        uniform_int_distribution<int> color_dist(0, 0xFFFFFF);

        for (int i = 0; i < n; i++) {
            int attempts = 0;
            bool placed = false;
            while (attempts < 3 && !placed) {
                double r = radius_dist(rng);
                // Ensure the atom is completely inside the window
                uniform_real_distribution<double> pos_x_dist(r, W - r);
                uniform_real_distribution<double> pos_y_dist(r, H - r);
                double x = pos_x_dist(rng);
                double y = pos_y_dist(rng);

                // Check for intersection with already placed atoms
                bool intersect = false;
                for (int j = 0; j < i; j++) {
                    double dx = atoms[j].x - x;
                    double dy = atoms[j].y - y;
                    double dist = sqrt(dx * dx + dy * dy);
                    if (dist < atoms[j].r + r) {
                        intersect = true;
                        break;
                    }
                }
                if (!intersect) {
                    atoms[i].r = r;
                    atoms[i].x = x;
                    atoms[i].y = y;
                    double speed = speed_dist(rng);
                    double angle = angle_dist(rng);
                    atoms[i].vx = speed * cos(angle);
                    atoms[i].vy = speed * sin(angle);
                    atoms[i].color = color_dist(rng);
                    placed = true;
                }
                attempts++;
            }
            if (!placed) {
                cerr << "Error: Could not place atom " << i
                    << " without intersection after 3 attempts." << endl;
                exit(1);
            }
        }
    }
    else if (argc == 2) {
        ifstream infile(argv[1]);
        if (!infile) {
            cerr << "Error: Cannot open file " << argv[1] << endl;
            exit(1);
        }
        int file_n;
        infile >> file_n;
        for (int i = 0; i < n; i++) {
            int color;
            double r, x, y, vx, vy;
            infile >> color >> r >> x >> y >> vx >> vy;
            if (!infile) {
                cerr << "Error: File format incorrect for atom " << i << endl;
                exit(1);
            }
            atoms[i].color = color;
            atoms[i].r = r;
            atoms[i].x = x;
            atoms[i].y = y;
            atoms[i].vx = vx;
            atoms[i].vy = vy;
        }
    }
    // Print the initial atom values (one per line)
    for (int i = 0; i < n; i++) {
        cout << atoms[i].color << " "
            << atoms[i].r << " "
            << atoms[i].x << " "
            << atoms[i].y << " "
            << atoms[i].vx << " "
            << atoms[i].vy << endl;
    }
}

//
// draw: Clears the window and draws each atom as a filled circle.
// Note: The drawing functions work with the top-left corner of the bounding rectangle,
// so we convert (center, radius) to (x-top, y-top) and width/height.
//
void draw(int n, Atom atoms[]) {
    // Clear screen by drawing a white rectangle covering the window
    fillRectangle(0, 0, W, H, 0xFFFFFF, NO_COLOR);

    for (int i = 0; i < n; i++) {
        int x_top = static_cast<int>(atoms[i].x - atoms[i].r);
        int y_top = static_cast<int>(atoms[i].y - atoms[i].r);
        int diameter = static_cast<int>(2 * atoms[i].r);
        fillEllipse(x_top, y_top, diameter, diameter, atoms[i].color, NO_COLOR);
    }
    flush();
}

//
// update: Updates the position of each atom, handles collisions with the window
// boundaries and then detects and resolves collisions between atoms.
// For boundary collisions, if an atom’s center is closer to a wall than its radius,
// the atom is repositioned and the corresponding velocity component is inverted.
// For atom–atom collisions, if two atoms overlap, we reposition one of them so that they
// just touch and then update the velocity components along the collision axis (using an
// elastic collision model with masses proportional to the square of the radii).
//
void update(int n, Atom atoms[]) {
    // Update positions and wall collisions
    for (int i = 0; i < n; i++) {
        atoms[i].x += atoms[i].vx;
        atoms[i].y += atoms[i].vy;

        // Left wall
        if (atoms[i].x - atoms[i].r <= 0) {
            atoms[i].x = atoms[i].r;
            atoms[i].vx = -atoms[i].vx;
        }
        // Right wall
        if (atoms[i].x + atoms[i].r >= W) {
            atoms[i].x = W - atoms[i].r;
            atoms[i].vx = -atoms[i].vx;
        }
        // Top wall
        if (atoms[i].y - atoms[i].r <= 0) {
            atoms[i].y = atoms[i].r;
            atoms[i].vy = -atoms[i].vy;
        }
        // Bottom wall
        if (atoms[i].y + atoms[i].r >= H) {
            atoms[i].y = H - atoms[i].r;
            atoms[i].vy = -atoms[i].vy;
        }
    }

    // Check collisions between atoms (for each pair i < j)
    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            double dx = atoms[j].x - atoms[i].x;
            double dy = atoms[j].y - atoms[i].y;
            double dist = sqrt(dx * dx + dy * dy);
            double sumR = atoms[i].r + atoms[j].r;
            if (dist < sumR) {
                // Reposition atom j so that the two atoms just touch
                double overlap = sumR - dist;
                double norm = (dist == 0) ? 1.0 : dist;
                atoms[j].x += (dx / norm) * overlap;
                atoms[j].y += (dy / norm) * overlap;

                // --- Collision resolution using elastic collision theory ---
                // Compute tangent vector (perpendicular to line joining centers)
                double tx = -dy;
                double ty = dx;
                double a = atan2(ty, tx);  // angle of the tangent

                // Get polar coordinates of velocities
                double vi_speed = sqrt(atoms[i].vx * atoms[i].vx + atoms[i].vy * atoms[i].vy);
                double vj_speed = sqrt(atoms[j].vx * atoms[j].vx + atoms[j].vy * atoms[j].vy);
                double vi_angle = atan2(atoms[i].vy, atoms[i].vx);
                double vj_angle = atan2(atoms[j].vy, atoms[j].vx);

                // Rotate velocities so that the tangent is horizontal
                double vi_rot_angle = vi_angle - a;
                double vj_rot_angle = vj_angle - a;

                // Decompose velocities in the rotated coordinate system
                double vi_horiz = vi_speed * cos(vi_rot_angle);
                double vi_vert = vi_speed * sin(vi_rot_angle);
                double vj_horiz = vj_speed * cos(vj_rot_angle);
                double vj_vert = vj_speed * sin(vj_rot_angle);

                // Masses proportional to the square of the radii
                double m1 = atoms[i].r * atoms[i].r;
                double m2 = atoms[j].r * atoms[j].r;
                // Compute center-of-mass velocity along the collision axis (vertical component)
                double V_center = (m1 * vi_vert + m2 * vj_vert) / (m1 + m2);
                // Compute new vertical velocities after collision (elastic collision)
                double vi_vert_new = 2 * V_center - vi_vert;
                double vj_vert_new = 2 * V_center - vj_vert;

                // Recombine with unchanged horizontal components
                double vi_rot_speed_new = sqrt(vi_horiz * vi_horiz + vi_vert_new * vi_vert_new);
                double vj_rot_speed_new = sqrt(vj_horiz * vj_horiz + vj_vert_new * vj_vert_new);
                double vi_rot_angle_new = atan2(vi_vert_new, vi_horiz);
                double vj_rot_angle_new = atan2(vj_vert_new, vj_horiz);

                // Rotate velocities back to original coordinate system
                double vi_final_angle = vi_rot_angle_new + a;
                double vj_final_angle = vj_rot_angle_new + a;
                atoms[i].vx = vi_rot_speed_new * cos(vi_final_angle);
                atoms[i].vy = vi_rot_speed_new * sin(vi_final_angle);
                atoms[j].vx = vj_rot_speed_new * cos(vj_final_angle);
                atoms[j].vy = vj_rot_speed_new * sin(vj_final_angle);
            }
        }
    }
}


//
// main: Creates the drawing window, initializes the atoms (either randomly or from file),
// draws the initial state, waits for the user to press Enter, then performs F update iterations
// with a delay of S milliseconds between each frame. Finally, it cleans up and waits until the
// user closes the window.
//
int main(int argc, const char* argv[])
{
    beginDrawing(W, H, "Atoms", 0xFFFFFF, false);
    int n = number(argc, argv);
    Atom* atoms = new Atom[n];
    init(n, atoms, argc, argv);
    draw(n, atoms);

    cout << "Press <ENTER> to continue..." << endl;
    string s;
    getline(cin, s);

    for (int i = 0; i < F; i++)
    {
        update(n, atoms);
        draw(n, atoms);
        this_thread::sleep_for(chrono::milliseconds(S));
    }

    delete[] atoms;
    cout << "Close window to exit..." << endl;
    endDrawing();
    return 0;
}
