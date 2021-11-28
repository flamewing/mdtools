/*
 * Copyright (C) Flamewing 2011-2015 <flamewing.sonic@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <getopt.h>
#include <mdtools/dplcfile.hh>
#include <mdtools/mappingfile.hh>

#include <array>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>

using std::cerr;
using std::endl;
using std::ifstream;
using std::ios;
using std::istream;
using std::ofstream;
using std::ostream;
using std::string;

static void usage() {
    cerr << "Usage: mapping_tool {-c|--crush-mappings} [OPTIONS] INPUT_MAPS "
            "OUTPUT_MAPS"
         << endl;
    cerr << "\tPerforms several size optimizations on the mappings file. All "
            "other options that write mappings also perform these"
         << endl
         << "\toptimizations, so this option should be used only if you don't "
            "want those side-effects."
         << endl
         << endl;
    cerr << "Usage: mapping_tool {-k|--crush-dplc} [OPTIONS] INPUT_DPLC "
            "OUTPUT_DPLC"
         << endl;
    cerr << "\tPerforms several size optimizations on the DPLC file. All other "
            "options that write DPLC files also perform these"
         << endl
         << "\toptimizations, so this option should be used only if you don't "
            "want those side-effects."
         << endl
         << endl;
    cerr << "Usage: mapping_tool {-o|--optimize} [OPTIONS] INPUT_MAPS "
            "INPUT_DPLC OUTPUT_MAPS OUTPUT_DPLC"
         << endl;
    cerr << "\tPerforms joint optimization on the input mappings and DPLC "
            "files to use as few DMA transfers as possible."
         << endl
         << endl;
    cerr << "Usage: mapping_tool {-s|--split} [OPTIONS] INPUT_MAPS OUTPUT_MAPS "
            "OUTPUT_DPLC"
         << endl;
    cerr << "\tConverts a mappings file into a mappings + DPLC pair, optimized "
            "to use as few DMA transfers as possible."
         << endl
         << endl;
    cerr << "Usage: mapping_tool {-m|--merge} [OPTIONS] INPUT_MAPS INPUT_DPLC "
            "OUTPUT_MAPS"
         << endl;
    cerr << "\tConverts a mappings + DPLC pair into a non-DPLC mappings file. "
            "Warning: this will not work correctly if there are"
         << endl
         << "\tmore than 0x1000 (4096) tiles in the art file." << endl
         << endl;
    cerr << "Usage: mapping_tool {-f|--fix} [OPTIONS] INPUT_MAPS OUTPUT_MAPS"
         << endl;
    cerr << "\tSonic 2 only. Fixes the SonMapED mappings file to the correct "
            "tile value for double resolution 2-player mode."
         << endl
         << endl;
    cerr << "Usage: mapping_tool {-p|--pal-change=SRC} {-a|--pal-dest=DST} "
            "[OPTIONS] INPUT_MAPS"
         << endl;
    cerr << "\tWrites new mappings with pieces matching the SRC palette bits "
            "changed to palette line DST."
         << endl
         << endl;
    cerr << "Usage: mapping_tool {-i|--info} [OPTIONS] INPUT_MAPS" << endl;
    cerr << "\tGives information about the mappings file." << endl << endl;
    cerr << "Usage: mapping_tool {-d|--dplc} [OPTIONS] INPUT_DPLC" << endl;
    cerr << "\tGives information about the DPLC file." << endl << endl;
    cerr << "\tWarning: the optimized mappings + DPLC files generated by this "
            "utility work perfectly in-game, but do not work in"
         << endl
         << "\tSonMapED. Complains about this should go to Xenowhirl." << endl
         << endl;
    cerr << "Available options are:" << endl;
    cerr << "\t-0, --no-null   \tDisables null first frame optimization. This "
            "affects both mappings and DPLC. It has no effects"
         << endl
         << "\t                \ton S3&K non-player DPLCs, which do not "
            "support this optimization. Disabling this optimization"
         << endl
         << "\t                \tincreases the odds of SonMapEd being able to "
            "load the mappings or DPLC files generated, but it"
         << endl
         << "\t                \tis not a guarantee. Complaints about SonMapEd "
            "should go to Xenowhirl."
         << endl;
    cerr << "\t--sonic=VER     \tShorthand for using both --from=sonic=VER and "
            "--to-sonic=VER."
         << endl;
    cerr << "\t--from-sonic=VER\tSpecifies the game engine version (format) "
            "for input mappings and DPLC."
         << endl;
    cerr << "\t--to-sonic=VER  \tSpecifies the game engine version (format) "
            "for output mappings and DPLC."
         << endl;
    cerr << "\t                \tThe following values are accepted for the "
            "game engine version:"
         << endl;
    cerr << "\t                \tVER=1\tSonic 1 mappings and DPLC." << endl;
    cerr << "\t                \tVER=2\tSonic 2 mappings and DPLC." << endl;
    cerr << "\t                \tVER=3\tSonic 3 mappings and DPLC, as used by "
            "player objects."
         << endl;
    cerr << "\t                \tVER=4\tSonic 3 mappings and DPLC, as used by "
            "non-player objects."
         << endl;
    cerr << "\t                \tInvalid values or unspecified options will "
            "default to Sonic 2 format all cases."
         << endl
         << endl;
}

enum Actions {
    eNone = 0,
    eOptimize,
    eSplit,
    eMerge,
    eFix,
    eConvert,
    eConvertDPLC,
    eInfo,
    eDplc,
    ePalChange
};

enum FileErrors {
    eInvalidArgs = 1,
    eInputMapsMissing,
    eInputDplcMissing,
    eOutputMapsMissing,
    eOutputDplcMissing
};

#define ARG_CASE(x, y, z, w)     \
    case (x):                    \
        if (act != eNone) {      \
            usage();             \
            return eInvalidArgs; \
        }                        \
        act   = (y);             \
        nargs = (z);             \
        w;                       \
        break;

#define TEST_FILE(x, y, z)                                                    \
    do {                                                                      \
        if (!(x).good()) {                                                    \
            cerr << "File '" << argv[(y)] << "' could not be opened." << endl \
                 << endl;                                                     \
            return (z);                                                       \
        }                                                                     \
    } while (0)

int main(int argc, char* argv[]) {
    constexpr static const std::array long_options{
            option{"optimize", no_argument, nullptr, 'o'},
            option{"split", no_argument, nullptr, 's'},
            option{"merge", no_argument, nullptr, 'm'},
            option{"fix", no_argument, nullptr, 'f'},
            option{"crush-mappings", no_argument, nullptr, 'c'},
            option{"crush-dplc", no_argument, nullptr, 'k'},
            option{"info", no_argument, nullptr, 'i'},
            option{"dplc", no_argument, nullptr, 'd'},
            option{"no-null", no_argument, nullptr, '0'},
            option{"pal-change", required_argument, nullptr, 'p'},
            option{"pal-dest", required_argument, nullptr, 'a'},
            option{"from-sonic", required_argument, nullptr, 'x'},
            option{"to-sonic", required_argument, nullptr, 'y'},
            option{"sonic", required_argument, nullptr, 'z'},
            option{nullptr, 0, nullptr, 0}};

    Actions act          = eNone;
    bool    nullfirst    = true;
    int64_t nargs        = 0;
    int64_t srcpal       = -1;
    int64_t dstpal       = -1;
    int64_t tosonicver   = 2;
    int64_t fromsonicver = 2;

    while (true) {
        int option_index = 0;
        int option_char  = getopt_long(
                 argc, argv, "osmfckidp:a:0", long_options.data(),
                 &option_index);
        if (option_char == -1) {
            break;
        }

        switch (option_char) {
        case '0':
            nullfirst = false;
            break;
        case 'x':
            fromsonicver = strtol(optarg, nullptr, 0);
            if (fromsonicver < 1 || fromsonicver > 4) {
                fromsonicver = 2;
            }
            break;
        case 'y':
            tosonicver = strtol(optarg, nullptr, 0);
            if (tosonicver < 1 || tosonicver > 4) {
                tosonicver = 2;
            }
            break;
        case 'z':
            fromsonicver = tosonicver = strtol(optarg, nullptr, 0);
            if (fromsonicver < 1 || fromsonicver > 4) {
                fromsonicver = tosonicver = 2;
            }
            break;
            ARG_CASE('o', eOptimize, 4, )
            ARG_CASE('s', eSplit, 3, )
            ARG_CASE('m', eMerge, 3, )
            ARG_CASE('f', eFix, 2, )
            ARG_CASE('c', eConvert, 2, )
            ARG_CASE('k', eConvertDPLC, 2, )
            ARG_CASE('i', eInfo, 1, )
            ARG_CASE('d', eDplc, 1, )
            ARG_CASE(
                    'p', ePalChange, 2,
                    srcpal = (strtoul(optarg, nullptr, 0) & 3U) << 5U)
        case 'a':
            if (act != ePalChange) {
                usage();
                return eInvalidArgs;
            }
            nargs  = 2;
            dstpal = (strtoul(optarg, nullptr, 0) & 3U) << 5U;
            break;
        default:
            break;
        }
    }

    if (argc - optind < nargs || act == eNone) {
        usage();
        return eInvalidArgs;
    }

    switch (act) {
    case eOptimize: {
        ifstream inmaps(argv[optind + 0], ios::in | ios::binary);
        ifstream indplc(argv[optind + 1], ios::in | ios::binary);
        TEST_FILE(inmaps, optind + 0, eInputMapsMissing);
        TEST_FILE(indplc, optind + 1, eInputDplcMissing);

        mapping_file srcmaps;
        srcmaps.read(inmaps, fromsonicver);
        inmaps.close();

        dplc_file srcdplc;
        srcdplc.read(indplc, fromsonicver);
        indplc.close();

        // mapping_file intmaps;
        // intmaps.merge(srcmaps, srcdplc);

        mapping_file dstmaps;
        dplc_file    dstdplc;
        // dstmaps.split(intmaps, dstdplc);
        dstmaps.optimize(srcmaps, srcdplc, dstdplc);

        ofstream outmaps(argv[optind + 2], ios::out | ios::binary | ios::trunc);
        ofstream outdplc(argv[optind + 3], ios::out | ios::binary | ios::trunc);
        TEST_FILE(outmaps, optind + 2, eOutputMapsMissing);
        TEST_FILE(outdplc, optind + 3, eOutputDplcMissing);

        dstmaps.write(outmaps, tosonicver, nullfirst);
        outmaps.close();

        dstdplc.write(outdplc, tosonicver, nullfirst);
        outdplc.close();
        break;
    }
    case eSplit: {
        ifstream inmaps(argv[optind + 0], ios::in | ios::binary);
        TEST_FILE(inmaps, optind + 0, eInputMapsMissing);

        mapping_file srcmaps;
        srcmaps.read(inmaps, fromsonicver);
        inmaps.close();

        mapping_file dstmaps;
        dplc_file    dstdplc = dstmaps.split(srcmaps);

        ofstream outmaps(argv[optind + 1], ios::out | ios::binary | ios::trunc);
        ofstream outdplc(argv[optind + 2], ios::out | ios::binary | ios::trunc);
        TEST_FILE(outmaps, optind + 1, eOutputMapsMissing);
        TEST_FILE(outdplc, optind + 2, eOutputDplcMissing);

        dstmaps.write(outmaps, tosonicver, nullfirst);
        outmaps.close();

        dstdplc.write(outdplc, tosonicver, nullfirst);
        outdplc.close();
        break;
    }
    case eMerge: {
        ifstream inmaps(argv[optind + 0], ios::in | ios::binary);
        ifstream indplc(argv[optind + 1], ios::in | ios::binary);
        TEST_FILE(inmaps, optind + 0, eInputMapsMissing);
        TEST_FILE(indplc, optind + 1, eInputDplcMissing);

        mapping_file srcmaps;
        srcmaps.read(inmaps, fromsonicver);
        inmaps.close();

        dplc_file srcdplc;
        srcdplc.read(indplc, fromsonicver);
        indplc.close();

        mapping_file dstmaps;
        dstmaps.merge(srcmaps, srcdplc);

        ofstream outmaps(argv[optind + 2], ios::out | ios::binary | ios::trunc);
        TEST_FILE(outmaps, optind + 2, eOutputMapsMissing);

        dstmaps.write(outmaps, tosonicver, nullfirst);
        outmaps.close();
        break;
    }
    case eFix: {
        ifstream inmaps(argv[optind + 0], ios::in | ios::binary);
        TEST_FILE(inmaps, optind + 0, eInputMapsMissing);

        mapping_file srcmaps;
        srcmaps.read(inmaps, fromsonicver);
        inmaps.close();

        ofstream outmaps(argv[optind + 1], ios::out | ios::binary | ios::trunc);
        TEST_FILE(outmaps, optind + 1, eOutputMapsMissing);

        srcmaps.write(outmaps, tosonicver, nullfirst);
        outmaps.close();
        break;
    }
    case eConvert: {
        ifstream inmaps(argv[optind + 0], ios::in | ios::binary);
        TEST_FILE(inmaps, optind + 0, eInputMapsMissing);

        mapping_file srcmaps;
        srcmaps.read(inmaps, fromsonicver);
        inmaps.close();

        ofstream outmaps(argv[optind + 1], ios::out | ios::binary | ios::trunc);
        TEST_FILE(outmaps, optind + 1, eOutputMapsMissing);

        srcmaps.write(outmaps, tosonicver, nullfirst);
        outmaps.close();
        break;
    }
    case eConvertDPLC: {
        ifstream indplc(argv[optind + 0], ios::in | ios::binary);
        TEST_FILE(indplc, optind + 0, eInputDplcMissing);

        dplc_file srcdplc;
        srcdplc.read(indplc, fromsonicver);
        indplc.close();

        ofstream outdplc(argv[optind + 1], ios::out | ios::binary | ios::trunc);
        TEST_FILE(outdplc, optind + 1, eOutputDplcMissing);

        srcdplc.write(outdplc, tosonicver, nullfirst);
        outdplc.close();
        break;
    }
    case eInfo: {
        ifstream inmaps(argv[optind + 0], ios::in | ios::binary);
        TEST_FILE(inmaps, optind + 0, eInputMapsMissing);

        mapping_file srcmaps;
        srcmaps.read(inmaps, fromsonicver);
        inmaps.close();
        srcmaps.print();
        break;
    }
    case eDplc: {
        ifstream indplc(argv[optind + 0], ios::in | ios::binary);
        TEST_FILE(indplc, optind + 0, eInputDplcMissing);

        dplc_file srcdplc;
        srcdplc.read(indplc, fromsonicver);
        indplc.close();
        srcdplc.print();
        break;
    }
    case ePalChange: {
        if (srcpal < 0 || dstpal < 0) {
            usage();
            return eInvalidArgs;
        }
        ifstream inmaps(argv[optind + 0], ios::in | ios::binary);
        TEST_FILE(inmaps, optind + 0, eInputMapsMissing);

        mapping_file srcmaps;
        srcmaps.read(inmaps, fromsonicver);
        inmaps.close();
        srcmaps.change_pal(srcpal, dstpal);

        ofstream outmaps(argv[optind + 1], ios::out | ios::binary | ios::trunc);
        TEST_FILE(outmaps, optind + 1, eOutputMapsMissing);

        srcmaps.write(outmaps, tosonicver, nullfirst);
        outmaps.close();
        break;
    }
    case eNone:
        cerr << "Divide By Cucumber Error. Please Reinstall Universe And "
                "Reboot."
             << endl;
        __builtin_unreachable();
    }

    return 0;
}
