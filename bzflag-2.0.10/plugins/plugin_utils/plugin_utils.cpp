/* bzflag
* Copyright (c) 1993 - 2007 Tim Riker
*
* This package is free software;  you can redistribute it and/or
* modify it under the terms of the license found in the file
* named COPYING that should have accompanied this file.
*
* THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
* IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
*/

#include <stdio.h>
#include <stdarg.h>
#include <sstream>
#include "plugin_utils.h"

std::string tolower(const std::string& s)
{
	std::string trans = s;

	for (std::string::iterator i=trans.begin(), end=trans.end(); i!=end; ++i)
		*i = ::tolower(*i);
	return trans;
}

std::string format(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	char	temp[2048];
	vsprintf(temp,fmt, args);
	std::string result = temp;
	va_end(args);
	return result;
}

std::vector<std::string> tokenize(const std::string& in, const std::string &delims, const int maxTokens, const bool useQuotes)
{
	std::vector<std::string> tokens;
	int numTokens = 0;
	bool inQuote = false;

	std::ostringstream currentToken;

	std::string::size_type pos = in.find_first_not_of(delims);
	int currentChar  = (pos == std::string::npos) ? -1 : in[pos];
	bool enoughTokens = (maxTokens && (numTokens >= (maxTokens-1)));

	while (pos != std::string::npos && !enoughTokens) {

		// get next token
		bool tokenDone = false;
		bool foundSlash = false;

		currentChar = (pos < in.size()) ? in[pos] : -1;
		while ((currentChar != -1) && !tokenDone){

			tokenDone = false;

			if (delims.find(currentChar) != std::string::npos && !inQuote) { // currentChar is a delim
				pos ++;
				break; // breaks out of while loop
			}

			if (!useQuotes){
				currentToken << char(currentChar);
			} else {

				switch (currentChar){
	  case '\\' : // found a backslash
		  if (foundSlash){
			  currentToken << char(currentChar);
			  foundSlash = false;
		  } else {
			  foundSlash = true;
		  }
		  break;
	  case '\"' : // found a quote
		  if (foundSlash){ // found \"
			  currentToken << char(currentChar);
			  foundSlash = false;
		  } else { // found unescaped "
			  if (inQuote){ // exiting a quote
				  // finish off current token
				  tokenDone = true;
				  inQuote = false;
				  //slurp off one additional delimeter if possible
				  if (pos+1 < in.size() &&
					  delims.find(in[pos+1]) != std::string::npos) {
						  pos++;
					  }

			  } else { // entering a quote
				  // finish off current token
				  tokenDone = true;
				  inQuote = true;
			  }
		  }
		  break;
	  default:
		  if (foundSlash){ // don't care about slashes except for above cases
			  currentToken << '\\';
			  foundSlash = false;
		  }
		  currentToken << char(currentChar);
		  break;
				}
			}

			pos++;
			currentChar = (pos < in.size()) ? in[pos] : -1;
		} // end of getting a Token

		if (currentToken.str().size() > 0){ // if the token is something add to list
			tokens.push_back(currentToken.str());
			currentToken.str("");
			numTokens ++;
		}

		enoughTokens = (maxTokens && (numTokens >= (maxTokens-1)));
		if (enoughTokens){
			break;
		} else{
			pos = in.find_first_not_of(delims,pos);
		}

	} // end of getting all tokens -- either EOL or max tokens reached

	if (enoughTokens && pos != std::string::npos) {
		std::string lastToken = in.substr(pos);
		if (lastToken.size() > 0)
			tokens.push_back(lastToken);
	}

	return tokens;
}



// Local Variables: ***
// mode:C++ ***
// tab-width: 8 ***
// c-basic-offset: 2 ***
// indent-tabs-mode: t ***
// End: ***
// ex: shiftwidth=2 tabstop=8
