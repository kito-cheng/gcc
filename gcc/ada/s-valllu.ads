------------------------------------------------------------------------------
--                                                                          --
--                         GNAT COMPILER COMPONENTS                         --
--                                                                          --
--                       S Y S T E M . V A L _ L L U                        --
--                                                                          --
--                                 S p e c                                  --
--                                                                          --
--          Copyright (C) 1992-2014, Free Software Foundation, Inc.         --
--                                                                          --
-- GNAT is free software;  you can  redistribute it  and/or modify it under --
-- terms of the  GNU General Public License as published  by the Free Soft- --
-- ware  Foundation;  either version 3,  or (at your option) any later ver- --
-- sion.  GNAT is distributed in the hope that it will be useful, but WITH- --
-- OUT ANY WARRANTY;  without even the  implied warranty of MERCHANTABILITY --
-- or FITNESS FOR A PARTICULAR PURPOSE.                                     --
--                                                                          --
-- As a special exception under Section 7 of GPL version 3, you are granted --
-- additional permissions described in the GCC Runtime Library Exception,   --
-- version 3.1, as published by the Free Software Foundation.               --
--                                                                          --
-- You should have received a copy of the GNU General Public License and    --
-- a copy of the GCC Runtime Library Exception along with this program;     --
-- see the files COPYING3 and COPYING.RUNTIME respectively.  If not, see    --
-- <http://www.gnu.org/licenses/>.                                          --
--                                                                          --
-- GNAT was originally developed  by the GNAT team at  New York University. --
-- Extensive contributions were provided by Ada Core Technologies Inc.      --
--                                                                          --
------------------------------------------------------------------------------

--  This package contains routines for scanning modular Long_Long_Unsigned
--  values for use in Text_IO.Modular_IO, and the Value attribute.

with System.Unsigned_Types;

package System.Val_LLU is
   pragma Pure;

   function Scan_Raw_Long_Long_Unsigned
     (Str : String;
      Ptr : not null access Integer;
      Max : Integer) return System.Unsigned_Types.Long_Long_Unsigned;
   --  This function scans the string starting at Str (Ptr.all) for a valid
   --  integer according to the syntax described in (RM 3.5(43)). The substring
   --  scanned extends no further than Str (Max).  Note: this does not scan
   --  leading or trailing blanks, nor leading sign.
   --
   --  There are three cases for the return:
   --
   --  If a valid integer is found, then Ptr.all is updated past the last
   --  character of the integer.
   --
   --  If no valid integer is found, then Ptr.all points either to an initial
   --  non-digit character, or to Max + 1 if the field is all spaces and the
   --  exception Constraint_Error is raised.
   --
   --  If a syntactically valid integer is scanned, but the value is out of
   --  range, or, in the based case, the base value is out of range or there
   --  is an out of range digit, then Ptr.all points past the integer, and
   --  Constraint_Error is raised.
   --
   --  Note: these rules correspond to the requirements for leaving the pointer
   --  positioned in Text_IO.Get. Note that the rules as stated in the RM would
   --  seem to imply that for a case like
   --
   --    8#12345670009#

   --  the pointer should be left at the first # having scanned out the longest
   --  valid integer literal (8), but in fact in this case the pointer points
   --  to the invalid based digit (9 in this case). Not only would the strict
   --  reading of the RM require unlimited backup, which is unreasonable, but
   --  in addition, the intepretation as given here is the one expected and
   --  enforced by the ACATS tests.
   --
   --  Note: if Str is empty, i.e. if Max is less than Ptr, then this is a
   --  special case of an all-blank string, and Ptr is unchanged, and hence
   --  is greater than Max as required in this case.
   --
   --  Note: this routine should not be called with Str'Last = Positive'Last.
   --  If this occurs Program_Error is raised with a message noting that this
   --  case is not supported. Most such cases are eliminated by the caller.

   function Scan_Long_Long_Unsigned
     (Str : String;
      Ptr : not null access Integer;
      Max : Integer) return System.Unsigned_Types.Long_Long_Unsigned;
   --  Same as Scan_Raw_Long_Long_Unsigned, except scans optional leading
   --  blanks, and an optional leading plus sign.
   --
   --  Note: if a minus sign is present, Constraint_Error will be raised.
   --  Note: trailing blanks are not scanned.

   function Value_Long_Long_Unsigned
     (Str : String) return System.Unsigned_Types.Long_Long_Unsigned;
   --  Used in computing X'Value (Str) where X is a modular integer type whose
   --  modulus exceeds the range of System.Unsigned_Types.Unsigned. Str is the
   --  string argument of the attribute. Constraint_Error is raised if the
   --  string is malformed, or if the value is out of range.

end System.Val_LLU;
