#!/bin/bash
## Charles Determan, 2015
##
## This is essentially mirrored from the BH package
## Given the size of ViennaCL compared to Boost may
## not need this script.

## Version Variables
##
## -- standard git checkout
pkgdir="${HOME}/RViennaCL"
## -- current ViennaCL version, placed eg in ${pkgdir}/local/
viennacltargz="ViennaCL-1.6.2.tar.gz"
date="2015-05-26"

## Internal constants/variables
## local directory
localdir="${pkgdir}/local"
## Derive the root name from the tarball
viennaclVer=$(basename ${viennacltargz} ".tar.gz")
## Create ViennaCL root directory name
viennaclRoot="${localdir}/${viennaclVer}"
## Create tarball file name with full path
viennaclSources="${localdir}/${viennacltargz}"
## target directory for headers
pkgincl="${pkgdir}/inst/include/ViennaCL"
## local files containing R package pieces
localfiles="${pkgdir}/local/files"

## Display current settings
echo "Using these settings:
Date:           ${date}
Version:        ${version}
viennacltargz:    ${viennacltargz}
PkgDir:         ${pkgdir}
PkgIncl:        ${pkgincl}
LocalDir:       ${localdir}
LocalFiles:     ${localfiles}
ViennaCLroot:     ${viennaclRoot}
ViennaCLsources:  ${viennaclSources}
"

## Sanity checks
if [ ! -f "${viennaclSources}" ]; then
    echo "ViennaCL input file ${viennaclSources} missing, exiting." 
    exit 1
fi

if [ -d ${ViennaCLroot} ]; then
    echo "Old ViennaCLroot directory exists, removing it (ie ${viennaclRoot})."
    rm -rf ${viennaclRoot}
fi


if [ 'ls -A ${pkgincl}' ]; then 
    echo "Include directory contains files, removing them.";
    rm -rf ${pkgincl}/*
fi

echo "Unpacking ${viennacltargz} into LocalDIR (ie ${localdir})."
(cd ${localdir} && tar xfz ${viennaclSources})

echo "Copying ViennaCL files from ${viennaclToot} into inst/incl Dir (ie ${pkgincl})"
cp -r ${viennaclRoot}/viennacl ${pkgincl}/viennacl> /dev/null 2>&1

## Post processing and cleanup
echo "Purging (temp. dir) ${viennaclRoot}"
rm -rf ${viennaclRoot}

echo "Now check with 'git status' and add and commit as needed."