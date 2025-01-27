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
#ifdef HAVE_EPIC

#include "InputGeneration/ep16Prep.H"

#include "AuxiliaryFunctions.H"

#include <iostream>


ep16Prep::~ep16Prep()
{
  delete _mdb;
}


ep16Prep *ep16Prep::readJSON(const std::string &ifname)
{
  std::cout << "Reading JSON file" << std::endl;
  std::ifstream inputStream(ifname);
  if (!inputStream.good() || nemAux::find_ext(ifname) != ".json")
  {
    std::cerr << "Error opening file " << ifname << std::endl;
    exit(1);
  }
  if (nemAux::find_ext(ifname) != ".json")
  {
    std::cerr << "Input File must be in .json format" << std::endl;
    exit(1);
  }

  jsoncons::json inputjson;
  inputStream >> inputjson;

  // checking if array
  if (inputjson.is_array())
  {
    std::cerr
        << "Warning: Input is an array. Only first element will be processed"
        << std::endl;
    inputjson = inputjson[0];
  }

  ep16Prep *ep = new ep16Prep(inputjson);
  ep->readJSON();
  return ep;
}


ep16Prep *ep16Prep::readJSON(const jsoncons::json &inputjson)
{
  ep16Prep *ep = new ep16Prep(inputjson);
  ep->readJSON();
  ep->process();
  ep->write();
  return ep;
}


void ep16Prep::readJSON()
{
  std::cout << "Reading Epic 2016 Input Generation JSON." << std::endl;

  // reading mandatory fields
  std::string type;
  _fname = _jstrm["File Name"].as<std::string>();
  if (_jstrm.contains("Type"))
  {
    type = _jstrm["Type"].as<std::string>();
    nemAux::toLower(type);
  }

  if (type != "short form"
      && type != "long form"
      && type != "long form edit")
  {
    std::cerr << "Unknown type " << type
              << ", switching to short form." << std::endl;
    type = "short form";
  }

  if (type == "long form")
  {
    std::cerr << "Long form is not supported yet." << std::endl;
    exit(1);
  }
  else if (type == "short form")
  {
    _shortForm = true;
  }

  // order definition
  _set_key(_fname);
  setNameType(_fname, inpFileType::INPGEN_TXT);
  std::vector<std::string> order;

  // ordering the process based on type
  if (type == "short form")
  {
    order = {
        "prep.case",
        "prep.run",
        "prep.exodus",
        "prep.arraysize",
        "mesh.nodeset.projectile",
        "mesh.nodeset.target",
        "mesh.elementset.projectile",
        "mesh.elementset.target",
        "misc.velocity",
        "misc.detonation"
    };
  }
  else if (type == "long form edit")
  {
    order = {
        "edit.read",
        "edit.slideline.seek",
        "edit.detonation",
        "edit.main",
        "edit.close"
    };
  }

  // set order
  setOrder(order);

  // other preps
  setCmntStr("$");
}


void ep16Prep::process()
{
  // Top level information
  if (_shortForm) {
    wrtCmnt("EPIC 2016 INPUT FILE");
    wrtCmnt("Generated on " + nemAux::getTimeStr() + " by NEMoSys");
    wrtCmnt("Short Form Description Card for ExodusII/CUBIT Data");

    // reading mesh databse
    std::string exo_fname = _jstrm["Mesh"]["File"].as<std::string>();
    _mdb = new NEM::MSH::EXOMesh::exoMesh(exo_fname);
    _mdb->read();

    // material information
    jsoncons::json _matj = _jstrm["Material"];
    for (const auto &km: _matj.object_range())
    {
      std::string kmkey = std::string(km.key());
      nemAux::toLower(kmkey);
      _mat[kmkey] = km.value().as<int>();
    }

    // boundary conditions information
    jsoncons::json _bcsj = _jstrm["Boundary Condition"];
    for (const auto &km: _bcsj.object_range()) {
      std::string kmkey = std::string(km.key());
      nemAux::toLower(kmkey);
      _bcs[kmkey] = km.value().as<std::string>();
      // translate to EPIC language
      std::string bcskmkey = _bcs[kmkey];
      nemAux::toLower(bcskmkey);
      if (bcskmkey == "fixed")
        _bcs[kmkey] = "111";
    }
  }

  // begin processing based on order specified
  std::vector<std::string> ord = getOrder();
  for (const auto &oi : ord)
  {
    std::string tsk = nemAux::findToStr(oi, ".");
    std::string _tsk = nemAux::findFromStr(oi, ".");
    std::string __tsk = nemAux::findFromStr(_tsk, ".");
    _tsk = nemAux::findToStr(_tsk, ".");
    std::cout << tsk << " -- " << _tsk << " -- " << __tsk << "\n";
    if (tsk == "prep")
      wrtPre(_tsk, __tsk);
    else if (tsk == "mesh")
      wrtMsh(_tsk, __tsk);
    else if (tsk == "misc")
      wrtMisc(_tsk, __tsk);
    else if (tsk == "edit")
      edit(_tsk, __tsk);
  }
}


void ep16Prep::wrtCmnt(const std::string &cmnt)
{
  std::stringstream ss;
  ss << getCmntStr() << cmnt;
  _write(ss);
}


void ep16Prep::wrtPre(const std::string &_tsk, const std::string &__tsk)
{
  std::stringstream _tcmnt;
  std::stringstream _tstr;
  if (_tsk == "case")
  {
    _tstr.clear();
    _tstr.str(std::string());
    _tcmnt.clear();
    _tcmnt.str(std::string());
    int _ctype = _jstrm["Case"]["Type"].as<int>();
    int _cid = _jstrm["Case"]["Id"].as<int>();
    std::string _des = " " + _jstrm["Case"]["Description"].as<std::string>();
    _tcmnt << "CASE DESCRIPTION";
    _tstr << std::setw(5) << _ctype
          << std::setw(5) << _cid
          << std::setw(70) << std::left << _des;
  }
  else if (_tsk == "run")
  {
    _tstr.clear();
    _tstr.str(std::string());
    _tcmnt.clear();
    _tcmnt.str(std::string());
    int _rmde, _unt, _pat, _dplt, _ssld;
    double _tmx, _cpmx;
    _pat = 3;
    _dplt = 3;
    _cpmx = 0.0;
    _rmde = _jstrm["Run"]["Mode"].as<int>();
    _unt = _jstrm["Run"]["Unit"].as<int>();
    _tmx = _jstrm["Run"]["Tmax"].as<double>();
    _ssld = _jstrm["Run"].get_with_default("SSLD", 1);
    _tcmnt << "Run Card\n";
    _tcmnt << getCmntStr() << " RUN UNIT  PAT DPLT      TMAX     CPMAX";
    _tstr.precision(4);
    _tstr << std::setw(5) << _rmde
          << std::setw(5) << _unt
          << std::setw(5) << _pat
          << std::setw(5) << _dplt
          << std::setw(10) << std::scientific << _tmx
          << std::setw(10) << _cpmx
          << std::setw(1) << _ssld;
  }
  else if (_tsk == "exodus")
  {
    // data gathering
    int _npns, _ntns, _npes, _ntes, _ctact, _conv, _sym, _tet;
    double _gap;
    std::vector<std::string> ns_names, eb_names;
    ns_names = _mdb->getNdeSetNames();
    eb_names = _mdb->getElmBlkNames();
    _npns = 0;
    _ntns = 0;
    _npes = 0;
    _ntes = 0;
    for (auto ni = ns_names.begin(); ni != ns_names.end(); ni++)
    {
      std::string pre = nemAux::findToStr(*ni, "_");
      nemAux::toLower(pre);
      if (pre == "prj")
        _npns++;
      else if (pre == "trg")
        _ntns++;
    }
    for (auto ei = eb_names.begin(); ei != eb_names.end(); ei++)
    {
      // skipping TRIANGULAR element blocks
      NEM::MSH::EXOMesh::elementType et = _mdb->getElmBlkType(*ei);
      if (et == NEM::MSH::EXOMesh::elementType::TRIANGLE)
      {
          std::cerr << "Warnning: TRIANGLE element blocks will be ignored!\n";
          continue;
      }

      std::string pre = nemAux::findToStr(*ei, "_");
      nemAux::toLower(pre);
      if (pre == "prj")
        _npes++;
      else if (pre == "trg")
        _ntes++;
    }
    _ctact = 1;
    _conv = 2;
    _sym = 0;
    _tet = 1;
    _gap = 0.1;

    _tstr.clear();
    _tstr.str(std::string());
    _tcmnt.clear();
    _tcmnt.str(std::string());
    _tcmnt << "Exodus II Description Card\n";
    _tcmnt << getCmntStr()
           << "NPNS NTNS NPES NTES CTCT CONV  SYM  TET       GAP";
    _tstr.precision(4);
    _tstr << std::setw(5) << _npns
          << std::setw(5) << _ntns
          << std::setw(5) << _npes
          << std::setw(5) << _ntes
          << std::setw(5) << _ctact
          << std::setw(5) << _conv
          << std::setw(5) << _sym
          << std::setw(5) << _tet
          << std::setw(10) << std::scientific << _gap;
  }
  else if (_tsk == "arraysize")
  {
    // data gathering
    int _mxn, _mxl, _mxmn, _mxsn;
    _mxn = _mdb->getNumberOfNodes();
    _mxl = _mdb->getNumberOfElements();
    _mxmn = _mxn;
    _mxsn = _mxn;

    _tstr.clear();
    _tstr.str(std::string());
    _tcmnt.clear();
    _tcmnt.str(std::string());
    _tcmnt << "Array Size/Dimension Card\n";
    _tcmnt << getCmntStr() << "      MXN       MXL      MXMN      MXSN";
    _tstr << std::setw(10) << _mxn * 50
          << std::setw(10) << _mxl * 50
          << std::setw(10) << _mxmn * 50
          << std::setw(10) << _mxsn * 50;
  }
  // write to stream
  if (!_tcmnt.str().empty())
    wrtCmnt(_tcmnt.str());
  if (!_tstr.str().empty())
    _write(_tstr);
}


void ep16Prep::wrtMsh(const std::string &_tsk, const std::string &__tsk)
{
  std::stringstream _tcmnt;
  std::stringstream _tstr;
  if (_tsk == "nodeset")
  {
    _tcmnt.clear();
    _tcmnt.str(std::string());
    _tstr.clear();
    _tstr.str(std::string());
    std::string preStr;
    if (__tsk == "projectile")
    {
      _tcmnt << "Projectile Node Set Cards\n";
      preStr = "prj";
    }
    else if (__tsk == "target")
    {
      _tcmnt << "Target Node Set Cards\n";
      preStr = "trg";
    }
    else
    {
      std::cerr << "Error: Unknown node set type " << __tsk << std::endl;
      throw;
    }
    _tcmnt << getCmntStr() << "NSET     XYZ";
    wrtCmnt(_tcmnt.str());
    for (int ns = 0; ns < _mdb->getNumberOfNodeSets(); ns++)
    {
      std::string pre = nemAux::findToStr(_mdb->getNdeSetName(ns), "_");
      nemAux::toLower(pre);
      if (pre == preStr)
      {
        int _nset = _mdb->getNdeSetId(ns);
        std::string nsName = _mdb->getNdeSetName(ns);
        nemAux::toLower(nsName);
        std::string nsChar;
        auto srch = _bcs.find(nsName);
        if (srch != _bcs.end())
          nsChar = srch->second;
        else
          nsChar = "000";
        _tstr << std::setw(5) << _nset << std::setw(8) << nsChar;
        _write(_tstr);
      }
    }
  }
  else if (_tsk == "elementset")
  {
    _tcmnt.clear();
    _tcmnt.str(std::string());
    _tstr.clear();
    _tstr.str(std::string());
    std::string preStr;
    if (__tsk == "projectile")
    {
      _tcmnt << "Projectile Element/Particle Set Cards\n";
      preStr = "prj";
    }
    else if (__tsk == "target")
    {
      _tcmnt << "Target Element/Particle Set Cards\n";
      preStr = "trg";
    }
    else
    {
      std::cerr << "Error: Unknown node set type " << __tsk << std::endl;
      throw;
    }
    _tcmnt << getCmntStr()
           << "LSET MATL TYPE SSET                                                  THICK/AREA";
    wrtCmnt(_tcmnt.str());
    int _lset, _matl, _type, _sset;
    double _ta;
    std::string _blnk;
    _blnk.insert(_blnk.end(), 50, ' ');
    for (int es = 0; es < (_mdb->getNumberOfElementBlocks()); es++)
    {
      std::string es_name = _mdb->getElmBlkName(es);
      nemAux::toLower(es_name);
      std::string pre = nemAux::findToStr(es_name, "_");
      if (pre == preStr)
      {
        _lset = _mdb->getElmBlkId(es);
        _matl = _mat[es_name];
        if (_matl == 0)
        {
          std::cerr << "WARNING: Keyword " << es_name
                    << " is not defined in input file." << std::endl;
          continue;
        }
        _type = 0;
        _sset = 0;
        _ta = 0.;
        _tstr.precision(4);
        _tstr << std::setw(5) << _lset
              << std::setw(5) << _matl
              << std::setw(5) << _type
              << std::setw(5) << _sset
              << _blnk
              << std::setw(10) << std::scientific << _ta;
        _write(_tstr);
      }
    }
  }
}


void ep16Prep::wrtMisc(const std::string &_tsk, const std::string &__tsk)
{
  std::stringstream _tcmnt;
  std::stringstream _tstr;
  if (_tsk == "velocity")
  {
    _tcmnt.clear();
    _tcmnt.str(std::string());
    _tcmnt << "Projectile and Target Velocities Card\n";
    _tcmnt << getCmntStr()
           << "    PXVEL     PYVEL     PZVEL     TXVEL     TYVEL     TZVEL";
    wrtCmnt(_tcmnt.str());

    // velocity information
    std::vector<double> _prjv, _trgv;
    jsoncons::json _vj = _jstrm["Velocity"];
    for (int i = 0; i < 3; i++)
    {
      _prjv.push_back(_vj["Projectile"][i].as<double>());
      _trgv.push_back(_vj["Target"][i].as<double>());
    }

    _tstr.clear();
    _tstr.str(std::string());
    _tstr.precision(8);
    _tstr << std::setw(10) << _prjv[0]
          << std::setw(10) << _prjv[1]
          << std::setw(10) << _prjv[2]
          << std::setw(10) << _trgv[0]
          << std::setw(10) << _trgv[1]
          << std::setw(10) << _trgv[2];
    _write(_tstr);
  }
  else if (_tsk == "detonation")
  {
    _tcmnt.clear();
    _tcmnt.str(std::string());
    _tcmnt << "Detonation Point Card\n";
    _tcmnt << getCmntStr() << "     XDET      YDET      ZDET";
    wrtCmnt(_tcmnt.str());

    // detonation information
    std::vector<double> _det;
    jsoncons::json _vj = _jstrm["Detonation"];
    for (int i = 0; i < 3; i++)
    {
      _det.push_back(_vj["Coordinate"][i].as<double>());
    }

    _tstr.clear();
    _tstr.str(std::string());
    _tstr.precision(8);
    _tstr << std::setw(10) << _det[0]
          << std::setw(10) << _det[1]
          << std::setw(10) << _det[2];
    _write(_tstr);
  }
}


void ep16Prep::edit(const std::string &_tsk, const std::string &__tsk)
{
  if (_tsk == "read")
    read(_fname);
  else if (_tsk == "slideline" && __tsk == "seek")
  {
    // slide line keyword check
    if (!_jstrm.contains("Slideline"))
      return;
    int seek = _jstrm["Slideline"].get_with_default("Seek", 6);

    std::stringstream iss;
    std::stringstream oss;
    iss << _buffer.rdbuf();
    while (iss.rdbuf()->in_avail() != 0)
    {
      char cl[81];
      iss.getline(cl, 81);
      std::string line(cl);
      if (line.find("seek") != std::string::npos)
      {
        // put back the line
        oss << line << "\n";
        int dmy;
        // copying nmg value
        iss >> dmy;
        oss << std::setw(5) << dmy;
        // change seek value to "seek"
        iss >> dmy;
        oss << std::setw(5) << seek;
        char rem[70];
        iss.read(rem, 70);
        oss << std::string(rem);
      }
      else
      {
        oss << line << "\n";
      }
    }
    // updating buffer
    _buffer.str(std::string());
    _buffer << oss.rdbuf();
  }
  else if (_tsk == "detonation")
  {
    // detonation information
    if (!_jstrm.contains("Detonation"))
      return;

    std::vector<double> _g;
    _g.resize(3, 0.0);
    double tburn = 0.0;

    if (_jstrm["Detonation"].contains("Gravity"))
    {
      _g[0] = _jstrm["Detonation"]["Gravity"][0].as<double>();
      _g[1] = _jstrm["Detonation"]["Gravity"][1].as<double>();
      _g[2] = _jstrm["Detonation"]["Gravity"][2].as<double>();
    }

    if (_jstrm["Detonation"].contains("Tburn"))
      tburn = _jstrm["Detonation"]["Tburn"].as<double>();

    std::stringstream iss;
    std::stringstream oss;
    iss << _buffer.rdbuf();
    while (iss.rdbuf()->in_avail() != 0)
    {
      char cl[81];
      iss.getline(cl, 81);
      std::string line(cl);
      if (line.find("rdet") != std::string::npos)
      {
        // put back the line
        oss << line << "\n";
        double dmy;
        int dmyi;
        // copying x/rdet value
        iss >> dmy;
        oss << std::setw(10) << std::setprecision(3) << std::scientific << dmy;
        // copying y/tdet value
        iss >> dmy;
        oss << std::setw(10) << std::setprecision(3) << std::scientific << dmy;
        // copying zdet value
        iss >> dmy;
        oss << std::setw(10) << std::setprecision(3) << std::scientific << dmy;
        // copying new tburn value
        iss >> dmy;
        oss << std::setw(10) << std::setprecision(3) << std::scientific << tburn;
        // copying shad value
        iss >> dmyi;
        oss << std::setw(5) << dmyi;
        // copying npnt value
        iss >> dmyi;
        oss << std::setw(5) << dmyi;
        // copying new xdd value
        iss >> dmy;
        oss << std::setw(10) << std::setprecision(3) << std::scientific << _g[0];
        // copying new ydd value
        iss >> dmy;
        oss << std::setw(10) << std::setprecision(3) << std::scientific << _g[1];
        // copying new zdd value
        iss >> dmy;
        oss << std::setw(10) << std::setprecision(3) << std::scientific << _g[2];
      }
      else
      {
        oss << line << "\n";
      }
    }
    // updating buffer
    _buffer.str(std::string());
    _buffer << oss.rdbuf();
  }
  else if (_tsk == "main")
  {
    // detonation information
    if (!_jstrm.contains("Main"))
      return;

    // set defaults to unused values
    int dplt = _jstrm["Main"].get_with_default("DPLT", -666);
    double dtdyn = _jstrm["Main"].get_with_default("DTDYN", -666.0);

    std::stringstream iss;
    std::stringstream oss;
    iss << _buffer.rdbuf();
    while (iss.rdbuf()->in_avail() != 0)
    {
      char cl[81];
      iss.getline(cl, 81);
      std::string line(cl);
      if (line.find("sys") != std::string::npos)
      {
        char *buf = new char[5];
        // put back the line
        oss << line << "\n";
        double dmy;
        int dmyi;

        // copying sys value
        iss >> dmyi;
        oss << std::setw(5) << dmyi;

        // copying nplt value
        iss >> dmyi;
        oss << std::setw(5) << dmyi;

        // copying lplt value
        iss >> dmyi;
        oss << std::setw(5) << dmyi;

        // copying dplt value
        iss.read(buf, 5);
        dmyi = std::stoi(std::string(buf));
        oss << std::setw(5) << (dplt == -666 ? dmyi : dplt);

        // copying dtsys value
        iss >> dmy;
        oss << std::setw(10) << std::setprecision(4) << std::scientific << dmy;

        // copying tsys value
        iss >> dmy;
        oss << std::setw(10) << std::setprecision(4) << std::scientific << dmy;

        // copying dtnode value
        iss >> dmy;
        oss << std::setw(10) << std::setprecision(4) << std::scientific << dmy;

        // copying tnode value
        iss >> dmy;
        oss << std::setw(10) << std::setprecision(4) << std::scientific << dmy;

        // copying dtdyn value
        iss >> dmy;
        oss << std::setw(10) << std::setprecision(4) << std::scientific
            << (dtdyn == -666 ? dmy : dtdyn);

        // copying tdyn value
        iss >> dmy;
        oss << std::setw(10) << std::setprecision(4) << std::scientific << dmy;

        delete[] buf;
      }
      else
      {
        oss << line << "\n";
      }
    }
    // updating buffer
    _buffer.str(std::string());
    _buffer << oss.rdbuf();
  }
  else if (_tsk == "close")
  {
    std::string ofname = _jstrm.get_with_default("Output File Name", _fname);
    close(ofname);
  }
}


void ep16Prep::read(const std::string &fname)
{
  std::ifstream _ifile;
  _ifile.open(fname);
  if (!_ifile.good())
  {
    std::cerr << "Error opening file " << fname << std::endl;
    throw;
  }
  _buffer.clear();
  _buffer << _ifile.rdbuf();
  //std::cout << "Buffer :\n" << _buffer.str() << std::endl;
  _ifile.close();
}


void ep16Prep::close(const std::string &fname)
{
  std::ofstream _ofile;
  _ofile.open(fname);
  if (!_ofile.is_open())
  {
    std::cerr << "Problem opening the output file." << std::endl;
    exit(1);
  }
  _ofile << _buffer.rdbuf();
  _ofile.close();
}

#endif
