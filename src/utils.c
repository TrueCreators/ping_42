#include "../include/ft_ping.h"
double ft_sqrt(double number) 
{
    if (number < 0) {
        fprintf(stderr, "Error: Cannot compute the square root of a negative number.\n");
        return -1; // Indicate error for negative inputs
    }
    if (number == 0 || number == 1) {
        return number; // Return early for 0 or 1
    }

    double guess = number / 2.0; // Initial guess
    double epsilon = 0.000001;  // Desired precision
    double next_guess;

    while (1) {
        next_guess = 0.5 * (guess + (number / guess)); // Newton-Raphson formula
        if ((next_guess - guess) < epsilon && (next_guess - guess) > -epsilon) {
            break; // Stop when the difference is within precision range
        }
        guess = next_guess;
    }

    return next_guess;
}