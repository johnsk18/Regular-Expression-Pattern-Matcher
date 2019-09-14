#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "debug.h"
#define LINE_LENGTH_MAX 1024

const char digits[10] = "0123456789";
const char letters[52] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

void error() { // outputs error
	perror("ERROR");
	exit(EXIT_FAILURE);
}

char* getRegex(const char** file) { // reads regex file and returns the regex string
	FILE* regexFile = fopen(*file, "r");
	if (regexFile == NULL) {
		fprintf(stderr, "ERROR: Invalid regex file format\n");
		exit(EXIT_FAILURE);
	}
	char* regexString = (char*)calloc(LINE_LENGTH_MAX, sizeof(char));
	if (fgets(regexString, LINE_LENGTH_MAX, regexFile) == NULL) error();
	regexString[strlen(regexString) - 1] = '\0';
	fclose(regexFile);
	return regexString;
}

char* regexSearch(const char* regex, const char* line); // regexSearch function prototype

char* zeroOrOne(const char* regex, const char* line, int i, int j, int* regexOffset, int* lineOffset, const char* target, int isMatchNon, int regexOffsetIncrement) { // tries to match zero or one target string in line
	(*regexOffset) += regexOffsetIncrement;
	if ((strchr(target, line[i+j-(*lineOffset)]) != NULL) ^ isMatchNon) {
		const char* r = regex + j+(*regexOffset)+1; // trimmed regex to test the matching of target
		const char* l = line + i+j-(*lineOffset)+1; // trimmed line to test the matching of target
		char* branch = regexSearch(r, l); // branches to test target match
		if (branch != NULL && branch == l) return (char*) line + i; // if not NULL, regex match found
	}
	(*lineOffset)++; // if branch failed, shifts line back to simulate zero target matched
	return NULL;
}

char* oneOrMore(const char* regex, const char* line, int i, int j, int* regexOffset, int* lineOffset, const char* target, int isMatchNon, int regexOffsetIncrement) { // tries to match one or more target string in line
	(*regexOffset) += regexOffsetIncrement;
	int i_ = i, found = 0;
	for ( ; ((strchr(target, line[i_+j-(*lineOffset)]) != NULL) ^ isMatchNon) && (line[i_+j-(*lineOffset)] != '\0'); i_++) found = 1;
	if (found) {
		for ( i_--; i_ >= i; i_--) {
			const char* r = regex + j+(*regexOffset)+1; // trimmed regex to test the matching of target
			const char* l = line + i_+j-(*lineOffset)+1; // trimmed line to test the matching of target
			char* branch = regexSearch(r, l); // branches to test target match
			if (branch != NULL && branch == l) return (char*) line + i; // if not NULL, regex match found
		}
	}
	return NULL;
}

char* zeroOrMore(const char* regex, const char* line, int i, int j, int* regexOffset, int* lineOffset, const char* target, int isMatchNon, int regexOffsetIncrement) { // tries to match zero or more target string in line
	(*regexOffset) += regexOffsetIncrement;
	int i_ = i, found = 0;
	for ( ; ((strchr(target, line[i_+j-(*lineOffset)]) != NULL) ^ isMatchNon) && (line[i_+j-(*lineOffset)] != '\0'); i_++) found = 1;
	if (found) {
		for ( i_--; i_ >= i; i_--) {
			const char* r = regex + j+(*regexOffset)+1; // trimmed regex to test the matching of target
			const char* l = line + i_+j-(*lineOffset)+1; // trimmed line to test the matching of target
			char* branch = regexSearch(r, l); // branches to test target match
			if (branch != NULL && branch == l) return (char*) line + i; // if not NULL, regex match found
		}
	}
	(*lineOffset)++; // if branch failed, shifts line back to simulate zero target matched
	return NULL;
}

char* groupTargetMore(const char* regex, const char* line, int i, int j, int* regexOffset, int* lineOffset, const char* closingBracket, int isZeroOrMore) {
	int i_ = i, j_ = j, found = 0, partFailed = 0;
	char* linePart = (char*)calloc(2, sizeof(char));
	char* regexPart = (char*)calloc(3, sizeof(char));
	(*regexOffset)++;
	do {
		do {
			partFailed = 0;
			if (regexPart[0]) { // if regexPart isn't null, meaning a match was found
				found = 1;
				partFailed = 1;
				break;
			}
			if (closingBracket - (regex + j_ + (*regexOffset)) == 0) break; // if end of group hit
			linePart[0] = line[i_+j-(*lineOffset)];
			regexPart[0] = regex[j_+(*regexOffset)];
			if (regexPart[0] == '\\') regexPart[1] = regex[j_+(*regexOffset)+1];
		} while (linePart[0] && regexSearch(regexPart, linePart));
		if (partFailed) i_++; // shift to next potential matching character
		else j_ += regex[j_+(*regexOffset)] == '\\' ? 2 : 1; // shift to next part in group
		memset(regexPart, '\0', 2);
	} while (closingBracket - (regex + j_ + (*regexOffset)) > 0);
	free(linePart);
	free(regexPart);
	if (found) { // if at least one match was found
		for (i_--; i_ >= i; i_--) { // in descending order of most matched characters, tries to match remaining regex
			const char* r = regex + j_ + (*regexOffset) + 2;
			const char* l = line + i_ + j - (*lineOffset) + 1;
			char* branch = regexSearch(r, l);
			if (branch != NULL && branch == l) return (char*) line + i;
		}
	}
	if (isZeroOrMore) { // if zero or more targets are trying to be matched, as opposed to one or more targets, shifts line back and regex forward to simulate zero target matched
		(*regexOffset) += j_ - j + 1;
		(*lineOffset)++;
	}
	return NULL;
}

char* regexSearch(const char* regex, const char* line) { // returns pointer to the start of matching regex, otherwise NULL
	if (!regex[0]) return (char*) line; // if regex is empty 
	int i = 0, j = 0, found = 0, lineOffset = 0, regexOffset = 0;
	for (i = 0, j = 0, lineOffset = 0, regexOffset = 0; i+j-lineOffset == 0 || line[i+j-lineOffset] != '\0'; i++) {
		for (j = 0, lineOffset = 0, regexOffset = 0, found = 0; regex[j+regexOffset] != '\0'; j++, found = 0) {
			if (regex[j+regexOffset] != '.') { // if not '.', which matches any character
				if (regex[j+regexOffset] == '\\') { // for handling regex syntax that starts with a '\'
					if (regex[j+regexOffset+1] == '\\') { // match a single literal backslash character '\'
						if (regex[j+regexOffset+2] == '?') { // match zero or one character
							char* result = zeroOrOne(regex, line, i, j, &regexOffset, &lineOffset, "\\", 0, 2);
							if (result) return result;
						} else if (regex[j+regexOffset+2] == '+') { // match one or more literal backslashes
							char* result = oneOrMore(regex, line, i, j, &regexOffset, &lineOffset, "\\", 0, 2);
							if (result) return result;
							break;
						} else if (regex[j+regexOffset+2] == '*') { // match zero or more of the same letters
							char* result = zeroOrMore(regex, line, i, j, &regexOffset, &lineOffset, "\\", 0, 2);
							if (result) return result;
						} else {
							if (line[i+j-lineOffset] != '\\') break; // if no repetition and match failed
							regexOffset++;
						}
					} else if (regex[j+regexOffset+1] == 'd') { // match any digit
						if (regex[j+regexOffset+2] == '?') { // match zero or one digit
							char* result = zeroOrOne(regex, line, i, j, &regexOffset, &lineOffset, digits, 0, 2);
							if (result) return result;
						} else if (regex[j+regexOffset+2] == '+') { // match one or more of the same digits
							char* result = oneOrMore(regex, line, i, j, &regexOffset, &lineOffset, digits, 0, 2);
							if (result) return result;
							break;
						} else if (regex[j+regexOffset+2] == '*') { // match zero or more of the same digits
							char* result = zeroOrMore(regex, line, i, j, &regexOffset, &lineOffset, digits, 0, 2);
							if (result) return result;
						} else { // match one digit
							if (strchr(digits, line[i+j-lineOffset]) == NULL) break;
							regexOffset++;
						}
					} else if (regex[j+regexOffset+1] == 'D') { // match any non-digit
						if (regex[j+regexOffset+2] == '?') { // match zero or one digit
							char* result = zeroOrOne(regex, line, i, j, &regexOffset, &lineOffset, digits, 1, 2);
							if (result) return result;
						} else if (regex[j+regexOffset+2] == '+') { // match one or more of the same non-digits
							char* result = oneOrMore(regex, line, i, j, &regexOffset, &lineOffset, digits, 1, 2);
							if (result) return result;
							break;
						} else if (regex[j+regexOffset+2] == '*') { // match zero or more of the same non-digits
							char* result = zeroOrMore(regex, line, i, j, &regexOffset, &lineOffset, digits, 1, 2);
							if (result) return result;
						} else { // match one non-digit
							if (strchr(digits, line[i+j-lineOffset])) break;
							regexOffset++;
						}
					} else if (regex[j+regexOffset+1] == 'w') { // match any letter
						if (regex[j+regexOffset+2] == '?') { // match zero or one letter
							char* result = zeroOrOne(regex, line, i, j, &regexOffset, &lineOffset, letters, 0, 2);
							if (result) return result;
						} else if (regex[j+regexOffset+2] == '+') { // match one or more of the same letters
							char* result = oneOrMore(regex, line, i, j, &regexOffset, &lineOffset, letters, 0, 2);
							if (result) return result;
							break;
						} else if (regex[j+regexOffset+2] == '*') { // match zero or more of the same letters
							char* result = zeroOrMore(regex, line, i, j, &regexOffset, &lineOffset, letters, 0, 2);
							if (result) return result;
						} else { // match one letter
							if (strchr(letters, line[i+j-lineOffset]) == NULL) break;
							regexOffset++;
						}
					} else if (regex[j+regexOffset+1] == 'W') { // match any non-letter
						if (regex[j+regexOffset+2] == '?') { // match zero or one letter
							char* result = zeroOrOne(regex, line, i, j, &regexOffset, &lineOffset, letters, 1, 2);
							if (result) return result;
						} else if (regex[j+regexOffset+2] == '+') { // match one or more of the same non-letters
							char* result = oneOrMore(regex, line, i, j, &regexOffset, &lineOffset, letters, 1, 2);
							if (result) return result;
							break;
						} else if (regex[j+regexOffset+2] == '*') { // match zero or more of the same non-letters
							char* result = zeroOrMore(regex, line, i, j, &regexOffset, &lineOffset, letters, 1, 2);
							if (result) return result;
						} else { // match one letter
							if (strchr(letters, line[i+j-lineOffset])) break;
							regexOffset++;
						}
					} else if (regex[j+regexOffset+1] == 's') { // match any whitespace
						if (regex[j+regexOffset+2] == '?') { // match zero or one whitespace
							char* result = zeroOrOne(regex, line, i, j, &regexOffset, &lineOffset, " \t", 0, 2);
							if (result) return result;
						} else if (regex[j+regexOffset+2] == '+') { // match one or more of the same whitespaces
							char* result = oneOrMore(regex, line, i, j, &regexOffset, &lineOffset, " \t", 0, 2);
							if (result) return result;
							break;
						} else if (regex[j+regexOffset+2] == '*') { // match zero or more of the same whitespaces
							char* result = zeroOrMore(regex, line, i, j, &regexOffset, &lineOffset, " \t", 0, 2);
							if (result) return result;
						} else { // match one whitespace
							if (strchr(" \t", line[i+j-lineOffset]) == NULL) break;
							regexOffset++;
						}
					}
				} else if (regex[j+regexOffset] == '[') { // match a group
					char* closingBracket = strchr(regex + j + regexOffset, ']'); // holds the location of the ending bracket
					if (closingBracket == NULL) return NULL; // ill-formed regex error case for a non-ending group bracket
					if (closingBracket[1] == '?') { // match zero or one group
						char* linePart = (char*)calloc(2, sizeof(char));
						char* regexPart = (char*)calloc(3, sizeof(char));
						for (regexOffset++; regex[j+regexOffset] != ']'; regexOffset++) {
							if (!found) {
								if (regex[j+regexOffset] == '\\') {
									strncpy(regexPart, regex + j + regexOffset, 2);
									regexOffset++;
								} else regexPart[0] = regex[j+regexOffset];
								linePart[0] = line[i+j-lineOffset];
								char* branch = regexSearch(regexPart, linePart);
								if (branch) found = 1;
								memset(regexPart, '\0', 2);
							}
						}
						free(linePart);
						free(regexPart);
						if (found) {
							const char* r = regex + j + regexOffset + 2;
							const char* l = line + i + j - lineOffset + 1;
							char* branch = regexSearch(r, l);
							if (branch != NULL && branch == l) return (char*) line + i;
						}
						regexOffset++;
						lineOffset++;
					} else if (closingBracket[1] == '+') { // match one or more group
						char* result = groupTargetMore(regex, line, i, j, &regexOffset, &lineOffset, closingBracket, 0);
						if (result) return result;
					} else if (closingBracket[1] == '*') { // match zero or more group
						char* result = groupTargetMore(regex, line, i, j, &regexOffset, &lineOffset, closingBracket, 1);
						if (result) return result;
					} else {
						char* linePart = (char*)calloc(2, sizeof(char));
						char* regexPart = (char*)calloc(3, sizeof(char));
						for (regexOffset++; regex[j+regexOffset] != ']'; regexOffset++) {
							if (!found) {
								if (regex[j+regexOffset] == '\\') strncpy(regexPart, regex + j + regexOffset, 2);
								else regexPart[0] = regex[j+regexOffset];
								linePart[0] = line[i+j-lineOffset];
								char* branch = regexSearch(regexPart, linePart);
								if (branch) found = 1;
								memset(regexPart, '\0', 2);
							}
						}
						free(linePart);
						free(regexPart);
						if (!found) break;
					}
				} else { // match character in line
					char* target = (char*)calloc(2, sizeof(char));
					target[0] = regex[j+regexOffset];
					if (regex[j+regexOffset+1] == '?') { // match zero or one character
						char* result = zeroOrOne(regex, line, i, j, &regexOffset, &lineOffset, target, 0, 1);
						free(target);
						if (result) return result;
					} else if (regex[j+regexOffset+1] == '+') { // match one or more of the same letters
						char* result = oneOrMore(regex, line, i, j, &regexOffset, &lineOffset, target, 0, 1);
						free(target);
						if (result) return result;
						break;
					} else if (regex[j+regexOffset+1] == '*') { // match zero or more of the same letters
						char* result = zeroOrMore(regex, line, i, j, &regexOffset, &lineOffset, target, 0, 1);
						free(target);
						if (result) return result;
					} else { // match one character
						free(target);
						if (line[i+j-lineOffset] != regex[j+regexOffset]) break;
					}
				}
			} else { // match any character
				if (regex[j+regexOffset+1] == '?') { // match zero or one of any character
					char* result = zeroOrOne(regex, line, i, j, &regexOffset, &lineOffset, "", 1, 1);
					if (result) return result;
				} else if (regex[j+regexOffset+1] == '+') { // match one or more of any characters
					char* result = oneOrMore(regex, line, i, j, &regexOffset, &lineOffset, "", 1, 1);
					if (result) return result;
					break;
				} else if (regex[j+regexOffset+1] == '*') { // match zero or more of any characters
					char* result = zeroOrMore(regex, line, i, j, &regexOffset, &lineOffset, "", 1, 1);
					if (result) return result;
				} // if no repetition, moves on in loop
			}
			// if end of line, returns reference to line element where regex starts
			if (!regex[j+regexOffset+1]) return (char*) line + i;
		}
	}
	return NULL;
}

/* regex_match() applies the given regex to each line of the file
 * specified by filename; all matching lines are stored in a
 * dynamically allocated array called matches; note that it is
 * up to the calling process to free the allocated memory here
 *
 * regex_match() returns the number of lines that matched (zero
 * or more) or -1 if an error occurred
 */
int regex_match(const char * filename, const char * regex, char *** matches) {
	FILE* inputFile = fopen(filename, "r");
	if (inputFile == NULL) {
		fprintf(stderr, "ERROR: Invalid input file format\n");
		return -1;
	}
  int lines = 0, matchCapacity = 16;
  *matches = (char**)calloc(matchCapacity, sizeof(char*));
	char* line = (char*)calloc(LINE_LENGTH_MAX, sizeof(char));
  for ( ; fgets(line, LINE_LENGTH_MAX, inputFile) != NULL; ) {
  	line[strlen(line) - 1] = '\0';
  	if (regexSearch(regex, line)) { // if regex match found, copies line to matches
			printf("%s\n", line);
			(*matches)[lines] = (char*)calloc(LINE_LENGTH_MAX, sizeof(char));
  		strcpy((*matches)[lines], line);
  		lines++;
  		if (lines == matchCapacity) { // if capacity reaches, adds 32 new spaces
  			matchCapacity += 32;
  			*matches = realloc(*matches, matchCapacity * sizeof(char*));
  		}
  	}
  }
  *matches = realloc(*matches, lines * sizeof(char*));
  free(line);
	fclose(inputFile);
  return lines;
}

int main(int argc, char const *argv[]) { 
	if (argc < 3)  { // error handling for command-line arguments
		fprintf(stderr, "ERROR: Invalid arugment(s)\nUSAGE: ./a.out <regex-file> <input-file>\n");
		exit(EXIT_FAILURE);
	}
	int i;
  char** matches;
	char* regex = getRegex(&argv[1]);
	int lines = regex_match(argv[2], regex, &matches);
  printf("\nThe number of lines matched is %d.\n", lines);
  for (i = 0; i < lines; i++) free(matches[i]);
  free(matches);
	free(regex);
  return EXIT_SUCCESS;
}
