/*******************************************************************************
* Promesh                                                                      *
* Copyright (C) 2022, IllinoisRocstar LLC. All rights reserved.                *
*                                                                              *
* Promesh is the property of IllinoisRocstar LLC.                              *
*                                                                              *
* IllinoisRocstar LLC                                                          *
* Champaign, IL                                                                *
* www.illinoisrocstar.com                                                      *
* promesh@illinoisrocstar.com                                                  *
*******************************************************************************/
/*******************************************************************************
* This file is part of Promesh                                                 *
*                                                                              *
* This version of Promesh is free software: you can redistribute it and/or     *
* modify it under the terms of the GNU Lesser General Public License as        *
* published by the Free Software Foundation, either version 3 of the License,  *
* or (at your option) any later version.                                       *
*                                                                              *
* Promesh is distributed in the hope that it will be useful, but WITHOUT ANY   *
* WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS    *
* FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more *
* details.                                                                     *
*                                                                              *
* You should have received a copy of the GNU Lesser General Public License     *
* along with this program. If not, see <https://www.gnu.org/licenses/>.        *
*                                                                              *
*******************************************************************************/
#ifndef NEMOSYS_AUXILIARYFUNCTIONS_H_
#define NEMOSYS_AUXILIARYFUNCTIONS_H_

#include <algorithm>
#include <chrono>
#include <cmath>
#include <ctime>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <random>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

//--------------------------Start nemAux namespace----------------------------//
namespace nemAux {

//----------------------------Auxiliary Classes-------------------------------//

// class wrapping around std::chrono for timing methods
class Timer {
 private:
  using time_t = std::chrono::time_point<std::chrono::system_clock>;

 public:
  Timer() : startTime(), stopTime() {}

  time_t start() { return startTime = std::chrono::system_clock::now(); }

  time_t stop() { return stopTime = std::chrono::system_clock::now(); }

  double elapsed() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(stopTime -
                                                                 startTime)
        .count();
  }

 private:
  time_t startTime, stopTime;
};

//----------------------------------------------------------------------------//

//----------------------Auxiliary Function Declarations-----------------------//

/*
Reasons for inline functions:

  Per C++11 Standard, every program (after linking) shall contain exactly one
  definition of every non-inline function or variable that is odr-used
  (one-definition-rule-used) in the program. An inline function shall be defined
  in every translation unit in which it is odr-used.

  So, if functions are non-inline and defined in a header that is included in
  multiple translation units (separate source files), the linker will see the
  same symbol is used more than once when merging object code. This violates the
  odr.

  If functions are inline and defined in a header, an exception is made to the
  odr rule, and it is defined in every translation unit in which it's used.
  Although inlining is a non-binding request, the inline keyword succeeds at
  telling the linker to tolerate multiple symbol definitions.
*/

// flattens vector of vectors
template <typename T>
std::vector<T> flatten(const std::vector<std::vector<T>> &v);

// folds vector into vector of vectors
template <typename T>
std::vector<std::vector<T>> fold(const std::vector<T> &v, int dim);

// adds two vectors
template <typename T>
std::vector<T> operator+(const std::vector<T> &x, const std::vector<T> &y);
// sub two vectors
template <typename T>
std::vector<T> operator-(const std::vector<T> &x, const std::vector<T> &y);

// multiplies vector by scalar
template <typename T>
std::vector<T> operator*(T a, const std::vector<T> &x);

// Hadamard product of vectors
template <typename T>
std::vector<T> hadamard(const std::vector<T> &x, const std::vector<T> &y);

// compute 2 norm of vec
template <typename T>
T l2_Norm(const std::vector<T> &x);
// computes reciprocal of argument
template <typename T>
T reciprocal(T x);
// IN PLACE, computes reciprocal of vector elements
template <typename T>
void reciprocal_vec(std::vector<T> &x);
// checks if vector has zeros
template <typename T>
bool hasZero(const std::vector<T> &x);
// find minmax of vector, excluding inf
template <typename T>
std::vector<T> getMinMax(const std::vector<T> &x);
// scales x from range [xmin, xmax] to within range [ymin, ymax]
// if x is inf, the scaled value will be ymax
template <typename T>
T scale_to_range(T x, const std::vector<T> &xminmax,
                 const std::vector<T> &yminmax);
// IN PLACE, scales each number in vector in range [xmin, xmax] to [ymin, ymax]
template <typename T>
void scale_vec_to_range(std::vector<T> &x, const std::vector<T> &xminmax,
                        const std::vector<T> &yminmax);
// get average and stdev of values
template <typename T>
std::vector<T> getMeanStdev(const std::vector<T> &x);
// generates boolean array with 1 if value >= tol, 0 otherwise
template <typename T>
std::vector<bool> cellsToRefine(const std::vector<T> &values, T tol);
// generates boolean array with 1 if value >= mean+/-dev, 0 otherwise
template <typename T>
std::vector<bool> cellsToRefineStdev(const std::vector<T> &values, T mean,
                                     T dev);
// return selects cells if their value is above some threshold of the maximum
template <typename T>
std::vector<bool> cellsToRefineMaxdev(const std::vector<T> &values, T dev);
// string trimming for consistent file names
inline std::string trim_fname(const std::string &name, const std::string &ext);
// find file extension
inline std::string find_ext(const std::string &fname);
// IN PLACE, to lower
inline void toLower(std::string &str);
// IN PLACE, to upper
inline void toUpper(std::string &str);
// return from beginning to the location of str
inline std::string findToStr(const std::string &str, const std::string &ptrn);
// return from the location of str
inline std::string findFromStr(const std::string &str, const std::string &ptrn);
// returns current time as string
inline std::string getTimeStr();
// find file extension
inline std::string find_name(const std::string &fname);
// print a vector
template <typename T>
void printVec(const std::vector<T> &v);
// find if in vector
template <typename T>
bool valInVec(const std::vector<T> &v, T val);
// flip a pair
template <typename A, typename B>
std::pair<B, A> flip_pair(const std::pair<A, B> &p);
// flip a map into a multimap
template <typename A, typename B>
std::multimap<B, A> flip_map(const std::map<A, B> &src);
// check if coordinate is inside a bounding box
template <typename T>
bool isInBBox(const std::vector<T> &crd, const std::vector<T> &bb);
// get a vector of the keys from a map (which are sorted)
template <typename A, typename B>
std::vector<A> getSortedKeys(const std::map<A, B> &mapObj);
// generate a random string of given length
inline std::string getRandomString(int length);
// converts string to character
inline std::shared_ptr<char> strToChar(const std::string &strng);
// decomposes string into tokens using single delimiter
inline std::vector<std::string> Tokenize(const std::string &lineIn,
                                         const char &delim);
inline std::vector<std::string> Tokenize(const std::string &lineIn,
                                         const std::string &delim);
// get the least positive integer that is not in the map and at least min
template <typename A, typename B>
A leastUnusedKey(const std::map<A, B> &map, A min = 1);
//----------------------------------------------------------------------------//

//-------------------Auxiliary Function Implementations-----------------------//

// flattens vector of vectors
template <typename T>
std::vector<T> flatten(const std::vector<std::vector<T>> &v) {
  std::size_t size = 0;
  for (const auto &sub : v) size += sub.size();

  std::vector<T> result;
  result.reserve(size);
  for (const auto &sub : v) result.insert(result.end(), sub.begin(), sub.end());
  return result;
}

// folds vector into vector of vectors, each of length dim
template <typename T>
std::vector<std::vector<T>> fold(const std::vector<T> &v, int dim) {
  if (v.size() % dim != 0) {
    std::cerr << "Size must be divisible by dim for folding" << std::endl;
    exit(1);
  }

  std::vector<std::vector<T>> result(v.size() / dim);
  for (std::size_t i = 0; i < result.size(); ++i) {
    result[i].resize(dim);
    for (std::size_t j = 0; j < dim; ++j) result[i][j] = v[i * dim + j];
  }
  return result;
}

// compute 2 norm of vec
template <typename T>
T l2_Norm(const std::vector<T> &x) {
  T result = 0.0;
  for (const auto &i : x) result += i * i;

  return std::sqrt(result);
}

// adds two vectors
template <typename T>
std::vector<T> operator+(const std::vector<T> &x, const std::vector<T> &y) {
  if (x.size() != y.size()) {
    std::cerr << "Vectors must be same length for addition" << std::endl;
    exit(1);
  }

  std::vector<T> result(x.size());
  for (std::size_t i = 0; i < x.size(); ++i) result[i] = x[i] + y[i];
  return result;
}

// sub two vectors
template <typename T>
std::vector<T> operator-(const std::vector<T> &x, const std::vector<T> &y) {
  if (x.size() != y.size()) {
    std::cerr << "Vectors must be same length for subtraction" << std::endl;
    exit(1);
  }

  std::vector<T> result(x.size());
  for (std::size_t i = 0; i < x.size(); ++i) result[i] = x[i] - y[i];
  return result;
}

// multiplies vector by scalar
template <typename T>
std::vector<T> operator*(T a, const std::vector<T> &x) {
  std::vector<T> result(x.size());
  for (std::size_t i = 0; i < x.size(); ++i) result[i] = a * x[i];
  return result;
}

template <typename T>
std::vector<T> hadamard(const std::vector<T> &x, const std::vector<T> &y) {
  if (x.size() != y.size()) {
    std::cerr << "Vectors must be same length for Hadamard product"
              << std::endl;
    exit(1);
  }

  std::vector<T> result(x.size());
  for (std::size_t i = 0; i < x.size(); ++i) result[i] = x[i] * y[i];
  return result;
}

// computes reciprocal of argument
template <typename T>
T reciprocal(T x) {
  return 1 / x;
}

// converts string to character for std::atof function
std::shared_ptr<char> strToChar(const std::string &strng) {
  std::shared_ptr<char> tab(new char[strng.length() + 1],
                            std::default_delete<char[]>());
  std::strcpy(tab.get(), strng.c_str());
  return tab;
}

// decomposes a string into tokens using single delimiter
std::vector<std::string> Tokenize(const std::string &lineIn,
                                  const char &delim) {
  std::vector<std::string> rtrnTokens;

  std::string intermediate;
  std::stringstream checkStr(lineIn);

  // Tokenizing
  while (std::getline(checkStr, intermediate, delim)) {
    rtrnTokens.push_back(intermediate);
  }
  return rtrnTokens;
}

// decomposes a string into tokens using string of delimiters
std::vector<std::string> Tokenize(const std::string &lineIn,
                                  const std::string &delim) {
  std::vector<std::string> rtrnTokens;
  std::vector<char> delimStr;
  delimStr.reserve(delim.size());

  for (int i = 0; i < delim.size(); i++) delimStr.push_back(delim[i]);

  std::string intermediate;
  std::stringstream checkStr(lineIn);

  // Tokenizing
  int iter = 0;
  while (std::getline(checkStr, intermediate, delimStr[iter])) {
    rtrnTokens.push_back(intermediate);
    iter++;
  }
  return rtrnTokens;
}

// IN PLACE, computes reciprocal of vector elements
template <typename T>
void reciprocal_vec(std::vector<T> &x) {
  std::transform(x.begin(), x.end(), x.begin(), reciprocal<T>);
}

template <typename T>
bool hasZero(const std::vector<T> &x) {
  for (const auto &i : x)
    if (i == 0) return true;
  return false;
}

// find minmax of vector, excluding inf
template <typename T>
std::vector<T> getMinMax(const std::vector<T> &x) {
  std::vector<T> tmp;
  for (std::size_t i = 0; i < x.size(); ++i)
    if (!std::isinf(x[i]))  // exclude inf
      tmp.push_back(x[i]);

  auto minmax = std::minmax_element(tmp.begin(), tmp.end());

  return {*minmax.first, *minmax.second};
}

// scales x from range [xmin, xmax] to within range [ymin, ymax]
// if x is inf, the scaled value will be ymax
template <typename T>
T scale_to_range(T x, const std::vector<T> &xminmax,
                 const std::vector<T> &yminmax) {
  if (std::isinf(x)) return yminmax[1];
  return yminmax[0] + (yminmax[1] - yminmax[0]) * (x - xminmax[0]) /
                          (xminmax[1] - xminmax[0]);
}

// IN PLACE, scales each number in vector in range [xmin, xmax] to [ymin,ymax]
template <typename T>
void scale_vec_to_range(std::vector<T> &x, const std::vector<T> &xminmax,
                        const std::vector<T> &yminmax) {
  for (auto &&i : x) i = scale_to_range(i, xminmax, yminmax);
}

// get average and stdev of values
template <typename T>
std::vector<T> getMeanStdev(const std::vector<T> &x) {
  T ave = 0;
  for (const auto &i : x) ave += i;
  ave /= x.size();

  T stdev = 0;
  for (const auto &i : x) stdev += (i - ave) * (i - ave);
  stdev = std::sqrt(stdev / x.size());

  return {ave, stdev};
}

// generates boolean array with 1 if value >= tol, 0 otherwise
template <typename T>
std::vector<bool> cellsToRefine(const std::vector<T> &values, T tol) {
  std::vector<bool> result(values.size(), false);
  for (std::size_t i = 0; i < values.size(); ++i)
    if (values[i] > tol) result[i] = true;
  return result;
}

// generates boolean array with true if value >= mean+/-dev, false otherwise
template <typename T>
std::vector<bool> cellsToRefineStdev(const std::vector<T> &values, T mean,
                                     T dev) {
  if (std::abs(dev / mean) >= 1.0) dev = 0.99 * mean;

  T th = mean + dev;
  T tl = mean - dev;

  std::vector<bool> result(values.size(), false);
  for (std::size_t i = 0; i < values.size(); ++i)
    if (values[i] > th || values[i] < tl) result[i] = true;
  return result;
}

// generates boolean array with 1 if value >= mean+/-dev, 0 otherwise
template <typename T>
std::vector<bool> cellsToRefineMaxdev(const std::vector<T> &values, T dev) {
  dev = std::abs(dev);
  if (dev > 1) dev = 1;

  T max = *std::max_element(values.begin(), values.end());
  T hl = (1 - dev) * max;

  std::vector<bool> result(values.size(), false);
  for (std::size_t i = 0; i < values.size(); ++i)
    if (values[i] > hl) result[i] = true;
  return result;
}

// string trimming for consistent file names
std::string trim_fname(const std::string &fname, const std::string &ext) {
  std::string::size_type end = fname.find_last_of('.');
  if (end != std::string::npos) {
    return fname.substr(0, end).append(ext);
  } else {
    std::cerr << "Error finding file extension for " << fname << std::endl;
    exit(1);
  }
}

// find file extension
std::string find_ext(const std::string &fname) {
  std::string::size_type last = fname.find_last_of('.');
  if (last != std::string::npos) {
    return fname.substr(last);
  } else {
    std::cerr << "Error finding file extension for " << fname << std::endl;
    exit(1);
  }
}

// IN PLACE, to lower
void toLower(std::string &str) {
  std::transform(str.begin(), str.end(), str.begin(), ::tolower);
}

// IN PLACE, to upper
void toUpper(std::string &str) {
  std::transform(str.begin(), str.end(), str.begin(), ::toupper);
}

// return from beginning to the location of str
std::string findToStr(const std::string &str, const std::string &ptrn) {
  return str.substr(0, str.find_first_of(ptrn));
}

// return from the location of str
std::string findFromStr(const std::string &str, const std::string &ptrn) {
  return str.substr(str.find_first_of(ptrn) + 1, std::string::npos);
}

// returns current time as string
std::string getTimeStr() {
  time_t rawTime;
  struct tm *timeInfo;
  char buffer[80];

  time(&rawTime);
  timeInfo = localtime(&rawTime);

  strftime(buffer, sizeof(buffer), "%d-%m-%Y %I:%M:%S %p", timeInfo);
  std::string str(buffer);

  return str;
}

// find file name
std::string find_name(const std::string &fname) {
  std::string::size_type first = fname.find_last_of('/');
  std::string::size_type last = fname.find_last_of('.');
  if (first != std::string::npos && last != std::string::npos) {
    return fname.substr(first + 1, last);
  } else {
    std::cerr << "error finding file extension for " << fname << std::endl;
    exit(1);
  }
}

// print vector
template <typename T>
void printVec(const std::vector<T> &v) {
  for (const auto &i : v) std::cout << std::setprecision(15) << i << " ";
  std::cout << std::endl;
}

// find is val is in vector
template <typename T>
bool valInVec(const std::vector<T> &v, T val) {
  for (const auto &i : v)
    if (i == val) return true;
  return false;
}

template <typename A, typename B>
std::pair<B, A> flip_pair(const std::pair<A, B> &p) {
  return std::pair<B, A>(p.second, p.first);
}

template <typename A, typename B>
std::multimap<B, A> flip_map(const std::map<A, B> &src) {
  std::multimap<B, A> dst;
  std::transform(src.begin(), src.end(), std::inserter(dst, dst.begin()),
                 flip_pair<A, B>);
  return dst;
}

template <typename T>
bool isInBBox(const std::vector<T> &crd, const std::vector<T> &bb) {
  for (std::size_t i = 0; i < crd.size(); ++i)
    if (crd[i] < bb[2 * i] || crd[i] > bb[2 * i + 1]) return false;
  return true;
}

template <typename A, typename B>
std::vector<A> getSortedKeys(const std::map<A, B> &mapObj) {
  if (!mapObj.empty()) {
    std::vector<A> sortedKeys(mapObj.size());
    for (const auto &i : mapObj) sortedKeys.emplace_back(i.first);
    return sortedKeys;
  } else {
    std::cerr << "Map is empty! No sorted keys to return." << std::endl;
    exit(1);
  }
}

// generate a random string of given length
std::string getRandomString(int length) {
  const char *alp = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
  std::random_device rd;
  std::default_random_engine dre(rd());
  std::uniform_int_distribution<int> uid(0, strlen(alp) - 1);

  std::string out;
  for (int i = 0; i < length; ++i) out += alp[uid(dre)];

  return out;
}

template <typename A, typename B>
A leastUnusedKey(const std::map<A, B> &map, A min) {
  A i = min;
  auto it = map.begin();
  if (min > 1) {
    auto ub = map.upper_bound(min);
    if (ub != it) {
      it = --ub;
    }
  }
  for (; it != map.end(); ++it) {
    if (it->first == i) {
      ++i;
    } else {
      return i;
    }
  }
  return i;
}

//---------------------------End nemAux namespace-----------------------------//
}  // namespace nemAux

#endif  // NEMOSYS_AUXILIARYFUNCTIONS_H_
