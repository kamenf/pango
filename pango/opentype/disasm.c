/* Pango
 * disasm.c: Dump OpenType layout tables
 *
 * Copyright (C) 2000 Red Hat Software
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <glib.h>		/* For G_HAVE_ISO_VARARGS */
#include <stdarg.h>

#include "disasm.h"

#ifdef G_HAVE_ISO_VARARGS
#define DUMP(...) dump (stream, indent, __VA_ARGS__)
#elif defined (G_HAVE_GNUC_VARARGS)
#define DUMP(args...) dump (stream, indent, args)
#endif
#define DUMP_FINT(strct,fld) dump (stream, indent, "<" #fld ">%d</" #fld ">\n", (strct)->fld)
#define DUMP_FUINT(strct,fld) dump (stream, indent, "<" #fld ">%u</" #fld ">\n", (strct)->fld)
#define DUMP_FGLYPH(strct,fld) dump (stream, indent, "<" #fld ">%#4x</" #fld ">\n", (strct)->fld)

#define DEF_DUMP(type) static void Dump_ ## type (TTO_ ## type *type, FILE *stream, int indent, FT_Bool is_gsub)
#define RECURSE(name, type, val) do {  DUMP ("<" #name ">\n"); Dump_ ## type (val, stream, indent + 1, is_gsub); DUMP ("</" #name ">\n"); } while (0)

static void
do_indent (FILE *stream, int indent)
{
  int i;

  for (i = 0; i < indent * 3; i++)
    fputc (' ', stream);
}

static void 
dump (FILE *stream, int indent, const char *format, ...) 
{
  va_list list;
  
  do_indent (stream, indent);
  
  va_start (list, format);
  vfprintf (stream, format, list);
  va_end (list);
}

static void 
Print_Tag (FT_ULong tag, FILE *stream)
{
  fprintf (stream, "%c%c%c%c", 
	   (unsigned char)(tag >> 24),
	   (unsigned char)((tag & 0xff0000) >> 16),
	   (unsigned char)((tag & 0xff00) >> 8),
	   (unsigned char)(tag & 0xff));
}

DEF_DUMP (LangSys)
{
  int i;

  DUMP_FUINT (LangSys, LookupOrderOffset);
  DUMP_FUINT (LangSys, ReqFeatureIndex);
  DUMP_FUINT (LangSys, FeatureCount);

  for (i=0; i < LangSys->FeatureCount; i++)
    DUMP("<FeatureIndex>%d</FeatureIndex>\n", LangSys->FeatureIndex[i]);
}

DEF_DUMP (Script)
{
  int i;

  RECURSE (DefaultLangSys, LangSys, &Script->DefaultLangSys);

  DUMP_FUINT (Script, LangSysCount);

  for (i=0; i < Script->LangSysCount; i++)
    {
      do_indent (stream, indent);
      fprintf (stream, "<LangSysTag>");
      Print_Tag (Script->LangSysRecord[i].LangSysTag, stream);
      fprintf (stream, "</LangSysTag>\n");
      RECURSE (LangSys, LangSys, &Script->LangSysRecord[i].LangSys);
    }
}

DEF_DUMP (ScriptList)
{
  int i;
  
  DUMP_FUINT (ScriptList, ScriptCount);

  for (i=0; i < ScriptList->ScriptCount; i++)
    { 
      do_indent (stream, indent);
      fprintf (stream, "<ScriptTag>");
      Print_Tag (ScriptList->ScriptRecord[i].ScriptTag, stream);
      fprintf (stream, "</ScriptTag>\n");
      RECURSE (Script, Script, &ScriptList->ScriptRecord[i].Script);
    }
}

DEF_DUMP (Feature)
{
  int i;
  
  DUMP_FUINT (Feature, FeatureParams);
  DUMP_FUINT (Feature, LookupListCount);

  for (i=0; i < Feature->LookupListCount; i++)
    DUMP("<LookupIndex>%d</LookupIndex>\n", Feature->LookupListIndex[i]);
}

DEF_DUMP (FeatureList)
{
  int i;
  
  DUMP_FUINT (FeatureList, FeatureCount);

  for (i=0; i < FeatureList->FeatureCount; i++)
    { 
      do_indent (stream, indent);
      fprintf (stream, "<FeatureTag>");
      Print_Tag (FeatureList->FeatureRecord[i].FeatureTag, stream);
      fprintf (stream, "</FeatureTag>\n");
      RECURSE (Feature, Feature, &FeatureList->FeatureRecord[i].Feature);
    }
}

DEF_DUMP (Coverage)
{
  DUMP_FUINT (Coverage, CoverageFormat);

  if (Coverage->CoverageFormat == 1)
    {
      int i;
      DUMP_FUINT (&Coverage->cf.cf1, GlyphCount);

      for (i = 0; i < Coverage->cf.cf1.GlyphCount; i++)
	DUMP("<Glyph>%#4x</Glyph> <!-- %d -->\n", Coverage->cf.cf1.GlyphArray[i], i);
    }
  else
    {
    }
}

static void
Dump_GSUB_Lookup_Single (TTO_SubTable *subtable, FILE *stream, int indent, FT_Bool is_gsub)
{
  TTO_SingleSubst *SingleSubst = &subtable->st.gsub.single;

  DUMP_FUINT (SingleSubst, SubstFormat);
  RECURSE (Coverage, Coverage, &SingleSubst->Coverage);

  if (SingleSubst->SubstFormat == 1)
    {
      DUMP_FINT (&SingleSubst->ssf.ssf1, DeltaGlyphID);
    }
  else
    {
      int i;
      
      DUMP_FINT (&SingleSubst->ssf.ssf2, GlyphCount);
      for (i=0; i < SingleSubst->ssf.ssf2.GlyphCount; i++)
	DUMP("<Substitute>%#4x</Substitute> <!-- %d -->\n", SingleSubst->ssf.ssf2.Substitute[i], i);
    }
}

DEF_DUMP (Ligature)
{
  int i;
  
  DUMP_FGLYPH (Ligature, LigGlyph);
  DUMP_FUINT (Ligature, ComponentCount);

  for (i=0; i < Ligature->ComponentCount - 1; i++)
    DUMP("<Component>%#4x</Component>\n", Ligature->Component[i]);
}

DEF_DUMP (LigatureSet)
{
  int i;
  
  DUMP_FUINT (LigatureSet, LigatureCount);

  for (i=0; i < LigatureSet->LigatureCount; i++)
    RECURSE (Ligature, Ligature, &LigatureSet->Ligature[i]);
}

static void
Dump_GSUB_Lookup_Ligature (TTO_SubTable *subtable, FILE *stream, int indent, FT_Bool is_gsub)
{
  int i;
  TTO_LigatureSubst *LigatureSubst = &subtable->st.gsub.ligature;

  DUMP_FUINT (LigatureSubst, SubstFormat);
  RECURSE (Coverage, Coverage, &LigatureSubst->Coverage);

  DUMP_FUINT (LigatureSubst, LigatureSetCount);

  for (i=0; i < LigatureSubst->LigatureSetCount; i++)
    RECURSE (LigatureSet, LigatureSet, &LigatureSubst->LigatureSet[i]);
}

DEF_DUMP (Lookup)
{
  int i;
  const char *lookup_name = NULL;
  void (*lookup_func) (TTO_SubTable *subtable, FILE *stream, int indent, FT_Bool is_gsub) = NULL;

  if (is_gsub)
    {
      switch (Lookup->LookupType)
	{
	case  GSUB_LOOKUP_SINGLE:
	  lookup_name = "SINGLE";
	  lookup_func = Dump_GSUB_Lookup_Single;
	  break;
	case  GSUB_LOOKUP_MULTIPLE:
	  lookup_name = "MULTIPLE";
	  break;
	case  GSUB_LOOKUP_ALTERNATE:
	  lookup_name = "ALTERNATE";
	  break;
	case  GSUB_LOOKUP_LIGATURE:
	  lookup_name = "LIGATURE";
	  lookup_func = Dump_GSUB_Lookup_Ligature;
	  break;
	case  GSUB_LOOKUP_CONTEXT:
	  lookup_name = "CONTEXT";
	  break;
	case  GSUB_LOOKUP_CHAIN:
	  lookup_name = "CHAIN";
	  break;
	}
    }
  else
    {
      switch (Lookup->LookupType)
	{
	case GPOS_LOOKUP_SINGLE:
	  lookup_name = "SINGLE";
	  break;
	case GPOS_LOOKUP_PAIR:
	  lookup_name = "PAIR";
	  break;
	case GPOS_LOOKUP_CURSIVE:
	  lookup_name = "CURSIVE";
	  break;
	case GPOS_LOOKUP_MARKBASE:
	  lookup_name = "MARKBASE";
	  break;
	case GPOS_LOOKUP_MARKLIG:
	  lookup_name = "MARKLIG";
	  break;
	case GPOS_LOOKUP_MARKMARK:
	  lookup_name = "MARKMARK";
	  break;
	case GPOS_LOOKUP_CONTEXT:
	  lookup_name = "CONTEXT";
	  break;
	case GPOS_LOOKUP_CHAIN:
	  lookup_name = "CHAIN";
	  break;
	}
    }

  DUMP("<LookupType>%s</LookupType>\n", lookup_name);

  for (i=0; i < Lookup->SubTableCount; i++)
    {
      DUMP ("<Subtable>\n");
      if (lookup_func)
	(*lookup_func) (&Lookup->SubTable[i], stream, indent + 1, is_gsub);
      DUMP ("</Subtable>\n");
    }
}

DEF_DUMP (LookupList)
{
  int i;

  DUMP_FUINT (LookupList, LookupCount);

  for (i=0; i < LookupList->LookupCount; i++)
    RECURSE (Lookup, Lookup, &LookupList->Lookup[i]);
}

void
TT_Dump_GSUB_Table (TTO_GSUB gsub, FILE *stream)
{
  int indent = 0;
  FT_Bool is_gsub = 1;

  RECURSE (ScriptList, ScriptList, &gsub->ScriptList);
  RECURSE (FeatureList, FeatureList, &gsub->FeatureList);
  RECURSE (LookupList, LookupList, &gsub->LookupList);
}

void
TT_Dump_GPOS_Table (TTO_GPOS gpos, FILE *stream)
{
  int indent = 0;
  FT_Bool is_gsub = 0;

  RECURSE (ScriptList, ScriptList, &gpos->ScriptList);
  RECURSE (FeatureList, FeatureList, &gpos->FeatureList);
  RECURSE (LookupList, LookupList, &gpos->LookupList);
}
