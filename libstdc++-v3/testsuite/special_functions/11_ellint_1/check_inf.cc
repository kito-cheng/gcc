// { dg-do run { target c++17 } }
// { dg-options "-D__STDCPP_WANT_MATH_SPEC_FUNCS__" }

// Copyright (C) 2026 Free Software Foundation, Inc.
//
// This file is part of the GNU ISO C++ Library.  This library is free
// software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the
// Free Software Foundation; either version 3, or (at your option)
// any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this library; see the file COPYING3.  If not see
// <http://www.gnu.org/licenses/>.

// 8.1.11 ellint_1 - non-finite phi must not produce UB (PR libstdc++/XXXXX)

#include <cmath>
#include <limits>
#include <stdexcept>
#include <testsuite_hooks.h>

void
test01()
{
  // +infinity phi should throw domain_error, not loop forever.
  bool caught = false;
  try
    {
      volatile float r = std::ellint_1f(0.5F,
				std::numeric_limits<float>::infinity());
      (void) r;
    }
  catch (const std::domain_error&)
    {
      caught = true;
    }
  VERIFY(caught);
}

void
test02()
{
  bool caught = false;
  try
    {
      volatile double r = std::ellint_1(0.5,
				std::numeric_limits<double>::infinity());
      (void) r;
    }
  catch (const std::domain_error&)
    {
      caught = true;
    }
  VERIFY(caught);
}

void
test03()
{
  bool caught = false;
  try
    {
      volatile long double r = std::ellint_1l(0.5L,
			std::numeric_limits<long double>::infinity());
      (void) r;
    }
  catch (const std::domain_error&)
    {
      caught = true;
    }
  VERIFY(caught);
}

int
main()
{
  test01();
  test02();
  test03();
  return 0;
}
