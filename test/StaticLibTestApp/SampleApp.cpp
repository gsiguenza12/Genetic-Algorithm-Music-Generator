/***************************
CS 471 - Genetic Algorithms for Music Generation
@author: Gabriel Alfredo Siguenza

version 5.0
****/

// Special thanks!
/*
	This is part of CFugue, a C++ Runtime for MIDI Score Programming
	Copyright (C) 2009 Gopalakrishna Palem

	For links to further information, or to contact the author,
	see <http://cfugue.sourceforge.net/>.
*/

// SampleApp.cpp
//
// Demonstrates the usage of CFugue as a Statically Linked Library
//
// CFugue is under active development.
// Please refer to documentation and latest releases at: http://cfugue.sourceforge.net/

#include "stdafx.h"
#include "stdlib.h"
#include <iostream>
#include <string>
#include "/projects/471_EC/include/CFugueLib.h" // specifies path for CFugue Library header file on my machine
#include <windows.h>
#include <string> 
#include <locale>
#include <codecvt>
#include <unordered_set>
#include <vector>


/*
DESIGN FOR IMPLEMENTATION:
	1. Read a MIDI file and represent it as a data structure.
	2. Define a fitness function to evaluate the quality of MIDI file.
	3. Implement genetic operations such as mutation and crossover.
	4. Implement the main algorithm loop.
*/
using namespace std;

// Set nPortID, and nTimerRes to defaults, can be overridden with cmd arguments
int nPortID = MIDI_MAPPER, nTimerRes = 20;

// In order to accurately measure intervals between the notes in the default octave (5) we need to map the notes in order.
// This will ensure that if there is a big leap between notes we can determine the value using the difference between the semitones.

// Initialize the map with semitone values
std::map<char, int> note_to_semitone = {
	{'C', 0}, {'D', 2}, {'E', 4},
	{'F', 5}, {'G', 7}, {'A', 9}, {'B', 11}
};


/*** These functions are part of the CFugue library for debugging the parser ***/
void OnParseTrace(const CFugue::CParser*, CFugue::CParser::TraceEventHandlerArgs* pEvArgs)
{
	std::wcout << "\n\t" << pEvArgs->szTraceMsg;
}

void OnParseError(const CFugue::CParser*, CFugue::CParser::ErrorEventHandlerArgs* pEvArgs)
{
	std::wcerr << "\n\t Error --> " << pEvArgs->szErrMsg;
	if (pEvArgs->szToken)
	{
		std::wcerr << "\t Token: " << pEvArgs->szToken;
	}
}
/*** End of CFugue Library parser functions ***/

/******************** Helper Functions ***************************/
/**
* Helper function to 'erase' all white space from a melody, to ensure that
* the fitness function does not score against empty notes
**/
string removeSpaces(const std::string& input) {
	std::string output = input;
	output.erase(std::remove(output.begin(), output.end(), ' '), output.end());
	return output;
}

/***
* Helper function to calculate the appropriate interval given two notes
***/
int calculateInterval(char note1, char note2) {
	// Get the semitone value for each note
	int semitone1 = note_to_semitone[note1];
	int semitone2 = note_to_semitone[note2];

	// Calculate and return the interval (absolute difference)
	return abs(semitone2 - semitone1);
}

/**
* Helper function to allow regular strings to be passed into CFugue functions,
* by converting string to microsoft specific type: TCHAR
**/
std::wstring stringToWstring(const std::string& s)
{
	std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
	return converter.from_bytes(s);
}

/**
* Helper function to seperate each character (notes) in a melody string
* with spaces to ensure playback in CFugue functions
**/
string separate_with_space(const string& str) {
	string result;
	for (char c : str) {
		result += c;
		result += ' ';
	}
	result.pop_back();  // remove trailing space
	return result;
}

/******************** End of Helper Functions ***************************/


// GENETIC ALGORITHM FUNCTIONS
/**
Generate Random Melody
**/
std::string generateNotes(int length) {
	std::string notes = "C D E F G A B";
	std::string generatedNotes = "";
	//srand(time(0));

	/*for (int i = 0; i < length; i++) {
		int index = rand() % 7 * 2;
		generatedNotes += notes.substr(index, 1);
		if (i != length - 1) {
			generatedNotes += " ";
		}
	}*/
	for (int i = 0; i < length; i++) {
		int index = rand() % 7 * 2;
		generatedNotes += notes.substr(index, 1);

		// Randomly decide whether to add a number from 0 to 10
		/*if (rand() % 2 == 0) {
			generatedNotes += std::to_string(rand() % 11);
		}
		*/
	
		if (i != length - 1) {
			generatedNotes += " ";
		}
	}

	return generatedNotes;
}

/**
* Calculates melody fitness. The parents for the next generation would be chosen based on their fitness,
* with higher fitness melodies having a higher chance of being selected.
* This function awards points for consonant intervals (unison, perfect fourth, perfect fifth),
* starting and ending on the tonic ('C'), and subtracts points for repeated notes to encourage diversity and adherence to tone
* reference: https://www.researchgate.net/publication/287009971_A_fitness_function_for_computer-generated_music_using_genetic_algorithms
**/
double fitness(const std::string& input) {
	double score = 0.0;
	unordered_set<char> unique_notes;
	int interval = 0;
	int octave = 0;

	// TODO: TEST THIS CHANGE 
	string melody = removeSpaces(input); // so that the current implementation of the fitness function does not score against ' '
	// Add points for each consonant interval
	for (int i = 0; i < melody.size() - 1; ++i) {
		// Logic for checking large jumps between notes (interval and octave)
		octave = 0; // reset octave
		if (isalpha(melody[i]) && isdigit(melody[i + 1])) {
			// Current character is a letter and the next character is a number
			//cout << "Found letter followed by number: " << melody[i] << melody[i + 1] << endl;
			octave = melody[i + 1]; // store the octave 
		}
		else {
			interval = calculateInterval(melody[i], melody[i + 1]);
		}
		//cout << " interval =" << interval;
		//cout << ", octave = " << octave << endl;

		if (interval == 0 || interval == 5 || interval == 7) { // Unison, perfect fourth, perfect fifth
			//cout << " points added for unison ";
			score += 1.0;
		}
		else if (interval == 4 || interval == 9) { // Major third, Major sixth
			//cout << " points added for major third, major sixth ";
			score += 0.7;
		}
		else if (interval == 12) { // Octave
			//cout << " points added for octave ";
			score += 1.5;
		}
		if (interval > 7) { // Penalize large jumps
			//cout << " points deducted for large jump ";
			score -= 0.5;
		}
		unique_notes.insert(melody[i]); // build out unique notes set

		// Add points for stepwise motion (small interval changes)
		if (interval == 1 || interval == 2) { // Half step or whole step
			//cout << " points added for small interval changes ";
			score += 0.6;
		}
	}

	// Add points for starting and ending on the tonic (in this case, 'C')
	if (melody.front() == 'C') {  
		//cout << " points added for starting on tonic C ";
		score += 1.0;
	}
	if (melody.back() == 'C') {
		//cout << " points added for ending on tonic C ";
		score += 1.0;
	}

	// Subtrac              t points for each repeated note
	for (int i = 0; i < melody.size() - 1; ++i) {
		if (melody[i] == melody[i + 1]) {
			//cout << " points deducted for repeated notes ";
			score -= 0.2;
		}
	}

	// Add points for variety of notes used
	//cout << " points totalled: " << score;
	score += 0.5 * unique_notes.size();
	//cout << "  possible additional points awarded for uniqueness ";

	return score;
}


/**
* randomly select a position within the string and change
* the note at that position to a different random note.
*
* examples of mutation algorithms:
* https://www.geeksforgeeks.org/mutation-algorithms-for-string-manipulation-ga/
**/
void mutate(std::string& melody) {
	// Define our musical notes
	const std::string notes = "ABCDEFG";

	// Create a version of melody without spaces
	std::string melodyNoSpaces = removeSpaces(melody);

	// Randomly select a position in the melody without spaces
	int positionNoSpaces = rand() % melodyNoSpaces.size();

	// Find the equivalent position in the original melody
	int position = 0;
	for (int i = 0, j = 0; i < melody.size() && j < positionNoSpaces; ++i) {
		if (melody[i] != ' ') {
			++j;
		}
		position = i;
	}

	// Randomly select a new note
	char new_note = notes[rand() % notes.size()];

	// Apply the mutation
	melody[position] = new_note;
}


/**
* Two strings are picked from the mating pool at random to
* crossover in order to produce superior offspring.
* reference: https://www.geeksforgeeks.org/crossover-in-genetic-algorithm/
**/
// TODO: Perform crossover between two melodies
std::pair<std::string, std::string> crossover(const std::string& parent1, const std::string& parent2) {
	// Make sure parents are the same size
	//assert(parent1.size() == parent2.size());

	// Randomly select a crossover point
	int crossover_point = rand() % parent1.size();

	// Create children by swapping subsequences after the crossover point
	std::string child1 = parent1.substr(0, crossover_point) + parent2.substr(crossover_point);
	std::string child2 = parent2.substr(0, crossover_point) + parent1.substr(crossover_point);

	return std::make_pair(child1, child2);
}


// Display a melody
void display_melody(string melody) { //const std::vector<Note>& melody
	// Implement melody display
	std::cout << melody;
	/*_tprintf(_T(melody));*/
}


/***
some fun CFugue functions:
These code snippets demo the CFugue software and serve as examples to build your music application
The following functions are part of the C API Tests
***/

void play_some_chords() {
	_tprintf(_T("\nPlaying Chrods.."));
	CFugue::PlayMusicStringWithOpts(_T("CMajH DMaj EMajH"), nPortID, nTimerRes);
	_tprintf(_T("    Done"));
}

void play_parallel() {
	_tprintf(_T("\nPlaying Parallel Notes.."));
	CFugue::PlayMusicStringWithOpts(_T("C+D+E+F+G+A+B"), nPortID, nTimerRes);
	_tprintf(_T("    Done"));
}

void play_sequential() {
	_tprintf(_T("\nPlaying Sequential Notes.."));
	CFugue::PlayMusicStringWithOpts(_T("B_A_G_F_E_D_C"), nPortID, nTimerRes);
	_tprintf(_T("    Done"));
}

void changeInstrumentDemo() {
	_tprintf(_T("\nChanging instrument to Flute and to Violin and back to Piano.."));
	CFugue::PlayMusicStringWithOpts(_T("C D I[Flute] E F I[Violin] G A I[PIANO] B C6"), nPortID, nTimerRes);
	_tprintf(_T("    Done"));
}

void playCarnatic() {
	// _tprintf(_T("\nPlaying Carnatic Music.."));

	_tprintf(_T("\nPlaying Kalyani.."));
	CFugue::PlayMusicStringWithOpts(_T("K[MELA_65] S R G M P D N S'"), nPortID, nTimerRes);
	_tprintf(_T("    Done"));

	_tprintf(_T("\nPlaying Kharaharapriya.."));
	CFugue::PlayMusicStringWithOpts(_T("K[MELA_22] S R G M P D N S'"), nPortID, nTimerRes);
	_tprintf(_T("    Done"));

	_tprintf(_T("\nPlaying Kalyani in Douple speed.."));
	CFugue::PlayMusicStringWithOpts(_T("K[MELA_65]S[2] S R G M P D N S'"), nPortID, nTimerRes);
	_tprintf(_T("    Done"));

	_tprintf(_T("\nPlaying Kharaharapriya in Triple speed.."));
	CFugue::PlayMusicStringWithOpts(_T("K[MELA_22]S[3] S R G M P D N S'"), nPortID, nTimerRes);
	_tprintf(_T("    Done"));
}

/*** The following functions are part of the C++ API Tests ***/
void runAll() {
	_tprintf(_T("\nPlaying Middle Octave.."));
	CFugue::Player player(nPortID, nTimerRes); // Create the Player Object
	player.Play(_T("C D E F G A B")); // Play the Music Notes on the default MIDI output port

	_tprintf(_T("\nPlaying few random notes.."));
	// Asynchronous Play
	if (player.PlayAsync(_T("Cx Dh Ah."))) // Start Playing Asynchronously
		while (player.IsPlaying()) // Wait while the play is in progress
			CFugue::MidiTimer::Sleep(1000);
	player.StopPlay(); // Match every PlayAsync with a StopPlay

	_tprintf(_T("\nSubscribing to Player parser events..."));
	// Subscribe to the Events
	player.Parser().evTrace.Subscribe(&OnParseTrace);
	player.Parser().evError.Subscribe(&OnParseError);

	_tprintf(_T("\nStarting to Play DDLJ Theme notes...\n"));
	// Play the Music Notes and then Save them to a MIDI File
	player.Play(_T("CI CI CI GI FI GI D#I. FI. G#I GI. RI ")
		_T("CI CI CI GI FI GI D#I. FI. D#I DI."));
	//if(false == player.SaveToMidiFile("PlayedOutput.mid"))
	//    _tprintf(_T("\n Unable to Save Played Music content to Midi Output File \n");
	_tprintf(_T("\n...Done !!...\n"));

	_tprintf(_T("\nStarting to Play Raghuvamsha Sudha notes...\n"));
	player.Play(_T("I[VIOLIN] ")
		_T("ci di f fi ei. di. ci ci cs b4s a4i g4i c ")
		_T("ci di f fi ei. di. ci ci cs b4s a4i g4i c ")
		_T("di fi a bi ei. gi. ")
		_T("ds fs ds fs ")
		_T("di fi a bi ei. gi. ")
		_T("c6 b ai c6s bs as gs fs es ds cs"));
	_tprintf(_T("\n...Done !!...\n"));
}


int main(int argc, char* argv[])
{
	srand(time(0)); // seed the current time for random generator
	const int population_size = 10; // set the population size to 10
	// e.g. the parents and run for several generations to simulate genetic mutation and crossover effects on subsequent generations (e.g. children)

	const int num_tracks = 2; // set the number of midi tracks to 2 
	const int generations = 1000; // run for 100 generations

	//_tprintf(_T("\nHello World!!\n\n"));
	_tprintf(_T("\n -------- Welcome to the Genetic Algo Music Program! ------------\n"));

	if (argc < 2)
	{
		unsigned int nOutPortCount = CFugue::GetMidiOutPortCount();
		if (nOutPortCount <= 0)
			exit(fprintf(stderr, "No MIDI Output Ports found !!"));
		if (nOutPortCount > 1)
		{
			_tprintf(_T("\n---------  The MIDI Out Port indices  --------\n\n"));
			/// List the MIDI Ports
			for (unsigned int i = 0; i < nOutPortCount; ++i)
			{
				std::string portName = CFugue::GetMidiOutPortName(i);
				std::wcout << "\t" << i << "\t: " << portName.c_str() << "\n";
			}
			/// Chose a Midi output port
			_tprintf(_T("\nChoose your MIDI Port ID for the play:"));
			_tscanf(_T("%d"), &nPortID);
		}
		else
		{
			std::string portName = CFugue::GetMidiOutPortName(0);
			std::wcout << "\nUsing the MIDI output port: " << portName.c_str();
		}
	}
	else if (argc == 2)
	{
		nPortID = atoi(argv[1]);
	}
	else if (argc > 2)
	{
		nPortID = atoi(argv[1]);
		nTimerRes = atoi(argv[2]);
	}

	// generate a string of 10 randomly generated notes
	string generatedNotes = generateNotes(10);

	string mel = "C D E F G A B";
	string mel2 = "B E G A B D A";
	string mel3 = "G F E D C B A";
	string mel4 = "";

#ifdef UNICODE // checks if UNICODE is defined
	std::wstring wmel = stringToWstring(mel); // call the string conversion function
	const TCHAR* melody = wmel.c_str(); // convert string melody into const TCHAR* to be used in the CFugue functions
#else
	const TCHAR* melody = mel.c_str(); // if unicode is not defined
#endif

	std::wstring wmel2 = stringToWstring(mel2); // call the string conversion function
	const TCHAR* melody2 = wmel2.c_str(); // convert string melody into const TCHAR* to be used in the CFugue functions

	std::wstring wmel3 = stringToWstring(mel3); // call the string conversion function
	const TCHAR* melody3 = wmel3.c_str(); // convert string melody into const TCHAR* to be used in the CFugue functions

	/////////////////////////////////////////////////////////////
	// Note the main loop seems to work fine, my assumption is that in this simple implementation
	// we are only mutating one random note each generation, we may end up with children that have lower 
	// fitness scores than their parents. Since genetics are much more complex than this in order to create advantageous evolutions
	// we would need to implement more genetic algorithms aside from simple mutations to get the very best fitness scores.
	// but will continue to test as scores seemed to only increase before my last code changes. 
	//////////////////////
	vector<string> population(population_size);

	// generate initial population 
	for (int i = 0; i < population_size; i++) {
		population[i] = generateNotes(12);
	}

	// start with two parents, modify the melodies using GA, compare offspring and improve melodies based on fitness values
	string parent1 = mel;
	string parent2 = mel2;

	double best_fitness = -100;
	double second_best_fitness = -100;
	string best_individual, second_best_individual;


	// select the two parents
	for (int i = 0; i < population_size; i++) {
		cout << "current melody: " << population[i] << endl;
		cout << "fitness of current melody: " << fitness(population[i]) << endl;
		double current_fitness = fitness(population[i]);
		if (current_fitness > best_fitness) {
			// Current individual has better fitness than the best so far
			// The old best becomes the second best
			second_best_fitness = best_fitness;
			second_best_individual = best_individual;

			best_fitness = current_fitness;
			best_individual = population[i];
		}
		else if (current_fitness > second_best_fitness) {
			// Current individual only has better fitness than the second best
			second_best_fitness = current_fitness;
			second_best_individual = population[i];
		}
		cout << "current best fitness: " << best_fitness << endl;
		cout << "current second best fitness: " << second_best_fitness << endl;
	}
	// set the two best parents from the population pool
	parent1 = best_individual;
	parent2 = second_best_individual;
	cout << "parent 1: " << parent1 << endl;
	cout << "parent 1 fitness score: " << fitness(parent1) << endl;
	cout << "playing parent 1 from gen 0: " << endl;
	std::wstring wmelpi1 = stringToWstring(parent1); // call the string conversion function
	const TCHAR* p1 = wmelpi1.c_str(); // convert string melody into const TCHAR* to be used in the CFugue functions
	CFugue::PlayMusicStringWithOpts(p1, nPortID, nTimerRes);

	cout << "parent 2: " << parent2 << endl;
	cout << "parent 2 fitness score: " << fitness(parent2) << endl;
	cout << "playing parent 2 from gen 0: " << endl;
	std::wstring wmelpi2 = stringToWstring(parent2); // call the string conversion function
	const TCHAR* p2 = wmelpi2.c_str(); // convert string melody into const TCHAR* to be used in the CFugue functions
	CFugue::PlayMusicStringWithOpts(p2, nPortID, nTimerRes);


	// run simulated generations, applying GA
	for (int i = 0; i < generations; i++) {
		// Generate new population
		for (int j = 0; j < population_size; j++) {
			// TODO: add another GA implementation, crossover. 
			// Create new melody by crossover
			// Perform crossover to generate children
			auto children = crossover(parent1, parent2);
			//population[j] = crossover(parent1, parent2); 
			// Apply mutation
			//mutate(population[j]);
			mutate(children.first);
			mutate(children.second);

			// update population with the best fit child
			if (fitness(children.first) > fitness(children.second)) {
				population[j] = children.first;
			}
			else {
				population[j] = children.second;
			}
			// population j for generation i has been set, 
			// selection has been implemented here as one of the crossover children has been eliminated.
		}
		cout << endl; // newline
		// Select the two best parents for the next generation
		best_fitness = -100;
		second_best_fitness = -100;

		// iterate through all of the children in the current population and find the best pair
		for (int j = 0; j < population_size; j++) {
			double current_fitness = fitness(population[j]);
			if (current_fitness > best_fitness) {
				// Current individual has better fitness than the best so far
				// The old best becomes the second best
				second_best_fitness = best_fitness;
				second_best_individual = best_individual;

				best_fitness = current_fitness;
				best_individual = population[j];
			}
			else if (current_fitness > second_best_fitness) {
				// Current individual only has better fitness than the second best
				second_best_fitness = current_fitness;
				second_best_individual = population[j];
			}
		}

		// Update parents for the next generation, best fit children become the best fit parents for subsequent generation
		parent1 = best_individual;
		cout << "current generation child 1: " << parent1;
		cout << " fitness score: " << fitness(parent1) << endl;
		parent2 = second_best_individual;
		cout << "current generation child 2: " << parent2;
		cout << " fitness score: " << fitness(parent2) << endl;

		// print the best melody and its fitness score in each generation
		cout << "Generation " << i << ": Best melody = " << parent1 << " with fitness = " << best_fitness << endl;
	}
	std::wstring wmelp1 = stringToWstring(parent1); // call the string conversion function
	const TCHAR* best = wmelp1.c_str(); // convert string melody into const TCHAR* to be used in the CFugue functions
	CFugue::PlayMusicStringWithOpts(best, nPortID, nTimerRes);

	// Uncomment the below to save notes as Midi file
	//_tprintf(_T("\nSaving to Midi file.."));
	//CFugue::SaveAsMidiFile(_T("C D E F G A B"), "output.mid");
	//_tprintf(_T("\tDone !!"));

	// make the program wait before closing
	cout << "Press any key to exit the program..." << endl;
	cin.get();
	return 0;
}