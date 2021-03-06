// HohmannPractice.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <vector>
#include <fstream>
#include <cmath>
#include <algorithm>

#include <boost/numeric/odeint.hpp>
#include <boost/math/constants/constants.hpp>

using namespace std;
using namespace boost::numeric::odeint;


/* Problem 3: Investigate the Sensitivities of a Hohmann Transfer
a) Compute a Hohmann transfer from a 200 km circular parking orbit to a circular GEO orbit
(altitude 35786 km). Assume the orbits are co-planar. What are the required v's? Simulate
this trajectory.

b) Model the rst impulse with a 1 error in the pointing direction (giving the v an ^r component). What errors does this cause in the transfer trajectory's orbital elements? If the second burn is performed nominally (same impulse as part a.) at apoapsis of the transfer trajectory, what errors are found in the nal orbit? Simulate this scenario and compare to part a.

c) Create a set of sensitivities for the nal state due to a pointing error from the data in part b. Using these sensitivities, calculate the expected errors in the nal state due to a 0.1 deg and a -0.3 deg error. Compare the expected errors calculated to simulation results.

d) Assume the vehicle can accelerate at a constant 30 m/s2. Model each of the nominal case burns as a finite burn with this acceleration. Explain exactly what you tested (burn times, ignition/cutoffconditions, thrust directions), and what differences in makes in comparison to part a. Note: there is no \right" answer here, i.e. you do not have to mess around until you almost hit the target orbit. I just want to see that you have explored the difference between a finite burn and an impulsive one.
*/

// Define constants
const double rEarth = 6378.1363;
const double muEarth = 3.986e5;

// Norm function
double norm(const vector<double> &vec)
{
	double result = 0.0;
	for (int i = 0; i < vec.size(); i++) {
		result += pow(vec[i],2);
	}
	result = sqrt(result);
	return result;
}

// Define planet struct
struct celestialBody
{
	celestialBody(double rad, double gravParam)
	{
		radius = rad;
		mu = gravParam;
	}
	double radius;
	double mu;
};

// Define state type
typedef vector<double> state;

// Create Hohmann Transfer Function
vector<double> computeHohmannImpulses(celestialBody &body, double initialRad, double finalRad)
{
	vector<double> hohmannResults(3);

	double transPeriRad = std::min(initialRad, finalRad);
	double transApoRad = std::max(initialRad, finalRad);
	double transSMA = 0.5*(initialRad + finalRad);
	double transPeriod = 2 * boost::math::constants::pi<double>()*sqrt(pow(transSMA, 3) / body.mu);

	double vMagInitial = sqrt(body.mu / initialRad);
	double vMagFinal = sqrt(body.mu / finalRad);
	double vMagTransPeri = sqrt((2 * body.mu / transPeriRad) - (body.mu / transSMA));
	double vMagTransApo = sqrt((2 * body.mu / transApoRad) - (body.mu / transSMA));

	if (initialRad < finalRad)
	{
		hohmannResults[0] = vMagTransPeri - vMagInitial;
		hohmannResults[1] = vMagFinal - vMagTransApo;
	}
	else
	{
		hohmannResults[0] = vMagTransApo - vMagInitial;
		hohmannResults[1] = vMagFinal - vMagTransPeri;
	}

	hohmannResults[2] = transPeriod;
	return hohmannResults;
}

// Define dynamical system
void TwoBodyDynamics(const celestialBody &body, const state &x, state &deriv, const double t)
{
	deriv[0] = x[3];
	deriv[1] = x[4];
	deriv[2] = x[5];

	vector<double> positionVector(x.begin(), x.begin() + 2);

	deriv[3] = body.mu / (pow(norm(positionVector), 3)) * positionVector[0];
	deriv[4] = body.mu / (pow(norm(positionVector), 3)) * positionVector[1];
	deriv[5] = body.mu / (pow(norm(positionVector), 3)) * positionVector[2];
}

// File observer to write out state data.
struct file_observer {
	ofstream &m_out;
	file_observer(ofstream &out) : m_out(out) {}

	void operator()(const state &x, double t) const {
		m_out << t;
		for (size_t i = 0; i < x.size(); i++) {
			m_out << "," << x[i];
		}
		m_out << "\n";
	}
};

struct time_history_observer
{
	vector<vector<double>> 
};

// Define type for stepper
typedef controlled_runge_kutta<runge_kutta_dopri5<state>> dopri_stepper_type;
dopri_stepper_type dopri_stepper;

int main()
{
	celestialBody Earth(rEarth, muEarth);

	vector<double> hohmannResults = computeHohmannImpulses(Earth, Earth.radius + 200.0, 35786.0);

	cout << "First burn: " << hohmannResults[0] << "\n";
	cout << "Second burn: " << hohmannResults[1] << "\n";
	cout << "Transfer orbit period: " << hohmannResults[2] << "\n";
	cout << "Now simulating...\n";

	state initialState(6); // state = [r v]^T
	initialState[0] = rEarth + 200.0;
	initialState[1] = initialState[2] = initialState[3] = initialState[5] = 0.0;
	initialState[4] = sqrt(Earth.mu / initialState[0]);

	const double dt = 0.1;

	ofstream myfile;
	myfile.open("output.csv");
	integrate_adaptive(dopri_stepper, [Earth](const state &x, state &deriv, const double t) {return TwoBodyDynamics(Earth, x, deriv, t);}, initialState, 0.0, hohmannResults[2], dt, file_observer(myfile));
	myfile.close();
}
