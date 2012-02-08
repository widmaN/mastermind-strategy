#include <iostream>
#include <random>

#include "Codeword.hpp"
#include "Algorithm.hpp"
#include "Engine.hpp"

using namespace Mastermind;

// Generates a random integer x such that 0 <= x < N.
static size_t randomNumber(size_t N)
{
	static std::mt19937 rng;
	return rng() % N;
}

// List all codewords.
static void listCodewords(
	CodewordList::const_iterator first,
	CodewordList::const_iterator last)
{
	for (CodewordList::const_iterator it = first; it != last; ++it)
	{
		std::cout << *it << ' '; // std::endl;
	}
}

// Display help screen.
static void displayHelp()
{
	std::cout << 
		"Available commands:\n"
		"    !          show the secret\n"
		"    d          list all codewords\n"
		"    g guess    make a guess\n"
		"    h          display this help screen\n"
		"    i          display information\n"
		"    l          list possibilities\n"
		"    q          quit\n"
		"    r          recommend a guess\n"
		"";
}

// Interactive mode.
int interactive(const CodewordRules &rules)
{
	// Set up default engine for the rules.
	Engine e(rules);

	// Display available commands.
	displayHelp();

	// Generate all codewords.
	std::cout << "Generating all codewords..." << std::endl;
	CodewordList all = e.generateCodewords();
	std::cout << "Done. There are " << all.size() << " codewords." << std::endl;

	// Generate a secret.
	Codeword secret = all[randomNumber(all.size())];
	std::cout << "Generated secret: " << secret << std::endl;

	while (1) 
	{
		std::cout << "> ";
		std::flush(std::cout);

		char c = 0;
		if (!(std::cin >> c) || c == 'q')
			break;

		Codeword guess = Codeword::emptyValue();
		switch (c)
		{
		case '!': // show the secret
			std::cout << "Secret: " << secret << std::endl;
			break;
		case 'd': // dump all codewords
			listCodewords(all.begin(), all.end());
			break;
		case 'h': // help
			displayHelp();
			break;
		case 'g': // guess
			if (!(std::cin >> guess))
			{
				std::cout << "Invalid codeword." << std::endl;
			}
			break;
		case 'l': // list possibilities
			break;
		default:
			std::cout << "Unknown command: " << c << std::endl;
			break;
		}

		//Codeword guess = Codeword::Parse(s.c_str(), rules);
		//if (guess.IsEmpty()) {
		//	cout << "Invalid input!" << endl;
		//} else {
		//	Feedback feedback = secret.CompareTo(guess);
		//	cout << feedback.ToString() << endl;
		//}
	}

#if 0
	// Generate a secret codeword.
	Codeword secret; // = Codeword::GetRandom(rules);
	printf("Generated secret: %s\n", secret.ToString().c_str());

	// Let's guess!
	//Feedback target(rules.length, 0);
	//CodewordList list = all;
	for (int k = 1; k <= 9; k++) {
		// Which codeword to guess? The first one.
		Codeword guess = list[0];

		Feedback fb = secret.CompareTo(guess);
		printf("[%d] Guess: %s, feedback: %s... ",
			k, guess.ToString().c_str(), fb.ToString().c_str());
		if (fb == target) {
			printf("That's it!\n");
			break;
		}

		CodewordList possibilities = list.FilterByFeedback(guess, fb);
		printf("possibilities: %d\n", possibilities.GetCount());
		list = possibilities;
	}

	system("PAUSE");
	return 0;

	// Generate a secret codeword.
	// Codeword secret = Codeword::GetRandom(rules);
	cout << "Fyi, the secret is " << secret.ToString() << endl;

	
#endif
	return 0;
}
