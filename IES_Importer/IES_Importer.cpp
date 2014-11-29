// IES_Importer.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"


#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_object.hpp>
#include <boost/spirit/include/phoenix_fusion.hpp>
#include <boost/spirit/include/phoenix_stl.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/include/vector.hpp>
#include <boost/fusion/include/io.hpp>

#include <boost/spirit/include/support_istream_iterator.hpp>


#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>




namespace IES {

  namespace qi = boost::spirit::qi;

  enum PhotometricType { TypeC = 1, TypeB = 2, TypeA = 3};
  struct photometric_type_ : qi::symbols<char, PhotometricType>
  {
    photometric_type_()
    {
      add
        ("1", TypeC)
        ("2", TypeB)
        ("3", TypeA)
        ;
    }

  } photometric_type;

  enum UnitsType { Feet = 1, Meter = 2 };
  struct units_type_ : qi::symbols<char, UnitsType>
  {
    units_type_()
    {
      add
        ("1", Feet)
        ("2", Meter)
        ;
    }

  } units_type;

  struct IES_Keyword {
    std::string name;
    std::string value;
  };



  struct IES_LuminousDimensions {
    UnitsType units_type;
    double width;
    double length;
    double height;
  };

  struct IES_LuminousShape {
    double ballast_factor;
    double future_use;
    double input_watts;
    std::vector<double> vertical_angles;
    std::vector<double> horizontal_angles;
    std::vector<double> candela_values;
  };

  struct IES_Settings {
    int number_of_lamps;
    double lumens_per_lamp;
    double multiplier;
    int number_of_vertical_angles;
    int number_of_horizontal_angles;
    PhotometricType photometric_type;
  };

  struct IES_File {
    std::string standard;
    std::vector<IES_Keyword> keywords;
    std::string tilt;
    IES_Settings settings;
    IES_LuminousDimensions dimensions;
    IES_LuminousShape shape;

    std::string manufacturer() const {
      return locate_keyword("MANUFAC");
    }

    std::string luminare_catalog_number() const {
      return locate_keyword("LUMCAT");
    }

    std::string lamp_catalog_number() const {
      return locate_keyword("LAMPCAT");
    }

    double width() const {
      return locate_keyword("LAMPCAT");
    }

    double height() const {
      return locate_keyword("LAMPCAT");
    }

    double length() const {
      return locate_keyword("LAMPCAT");
    }

  private:
    std::string locate_keyword(const std::string& keyword_name) const {
      auto it = std::find_if(keywords.cbegin(), keywords.cend(), [&keyword_name](const IES_Keyword& kw) {
        return kw.name == keyword_name;
      });
      if (it != keywords.end()) {
        return it->value;
      }
      else {
        return "";
      }
    }
  };
}
BOOST_FUSION_ADAPT_STRUCT(
  IES::IES_Keyword,
  (std::string, name)
  (std::string, value)
  )

BOOST_FUSION_ADAPT_STRUCT(
  IES::IES_LuminousDimensions,
  (IES::UnitsType, units_type)
(double, width)
(double, length)
(double, height)
  )

BOOST_FUSION_ADAPT_STRUCT(
IES::IES_LuminousShape,
(double, ballast_factor)
(double, future_use)
(double, input_watts)
(std::vector<double>, vertical_angles)
(std::vector<double>, horizontal_angles)
(std::vector<double>, candela_values)
)

BOOST_FUSION_ADAPT_STRUCT(
IES::IES_Settings,
(int, number_of_lamps)
(double, lumens_per_lamp)
(double, multiplier)
(int, number_of_vertical_angles)
(int, number_of_horizontal_angles)
(IES::PhotometricType, photometric_type)

)


BOOST_FUSION_ADAPT_STRUCT(
  IES::IES_File,
  (std::string, standard)
  (std::vector<IES::IES_Keyword>, keywords)
  (std::string, tilt)
  (IES::IES_Settings, settings)
  (IES::IES_LuminousDimensions, dimensions)
  (IES::IES_LuminousShape, shape)
  )

namespace IES {

  namespace ascii = boost::spirit::ascii;
  namespace phoenix = boost::phoenix;

  template <typename Iterator>
  struct IES_File_parser : qi::grammar<Iterator, IES_File(), ascii::space_type>
  {
    IES_File_parser() : IES_File_parser::base_type(start)
    {
      using qi::lexeme;
      using qi::lit;
      using qi::double_;
      using qi::int_;

      using boost::spirit::qi::repeat;
      using ascii::char_;

      using namespace qi::labels;

      namespace phx = boost::phoenix;

      word %= lexeme[+(char_ - '\n')];


      standard %= word;

      keyword %= '['
        >> lexeme [+ (char_ - ']')]
        >> ']'
        >> word
        ;

      dimensions %= units_type
        >> double_
        >> double_
        >> double_;

      //shape

      tilt %= lit("TILT=") >> word;

      int n_vert_angs = 0;
      int n_hor_angs = 0;

      settings %= int_ //number_of_lamps
        >> double_ //lumens_per_lamp
        >> double_ //multiplier
        >> int_[phx::ref(n_vert_angs) = _1] //number_of_vertical_angles
        >> int_[phx::ref(n_hor_angs) = _1] //number_of_horizontal_angles
        >> photometric_type;//photometric_type



      shape %= double_ >> double_ >> double_
        >> repeat(phx::ref(n_vert_angs))[double_] //vertical_values
        >> repeat(phx::ref(n_hor_angs))[double_] //horizontal_values
        >> *double_ //candela_values
        ;

      start %=
        //lit("IES_File")
        standard
      //  >> 
        >> *keyword
        >> tilt
        >> settings
        >> dimensions
        >> shape
        ;
    }
    qi::rule<Iterator, std::string(), ascii::space_type> standard;
    qi::rule<Iterator, IES_File(), ascii::space_type> start;
    qi::rule<Iterator, IES_Settings(), ascii::space_type> settings;
    

    qi::rule<Iterator, IES_Keyword(), ascii::space_type> keyword;
    qi::rule<Iterator, IES_LuminousDimensions(), ascii::space_type> dimensions;
    qi::rule<Iterator, IES_LuminousShape(), ascii::space_type> shape;
    qi::rule<Iterator, std::string(), ascii::space_type> tilt;

    qi::rule<Iterator, std::string(), ascii::space_type> word;
  };
}



class IESLoader {
  IES::IES_File _iesFile;
public:
  bool load(const char* path) {
    namespace spirit = boost::spirit;

    std::ifstream inf(path);
    inf.unsetf(std::ios::skipws);

    spirit::istream_iterator begin(inf);
    spirit::istream_iterator end;

    using boost::spirit::ascii::space;
    //typedef std::string::const_iterator iterator_type;
    typedef IES::IES_File_parser<spirit::istream_iterator> IES_File_parser;

    IES_File_parser g;

    bool ok = phrase_parse(begin, end, g, space, _iesFile);
    std::cout << "Manufacturer is: " << _iesFile.manufacturer() << "." << std::endl;
    return ok;
  }


};

int _tmain(int argc, _TCHAR* argv[])
{
  IESLoader iesLoader;
  bool ok = iesLoader.load("11790_BZ.ies");
  std::cin.ignore(1);
	return 0;
}

