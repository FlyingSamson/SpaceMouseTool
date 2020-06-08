#!/bin/bash

shopt -s extglob

GIT=git
MKDIR=mkdir
CD=cd
CP=cp
ZIP=zip
TAR=tar

DEBUG=0

function usage() {
    echo "Usage: $0 -t <tag> [-h|--help] [-d|--debug]";
}

function debug() {
    if [[ DEBUG -eq 1 ]]; then
	echo $1
    fi
}

long_args="";         short_args=""
long_args+="tag:,";   short_args+="t:,"
long_args+="help,";     short_args+="h,"
long_args+="debug,";     short_args+="d,"

# read in arguments using getopt
opts=$(getopt --longoptions ${long_args} --options ${short_args} --name "$(basename $0)" -- "$@")

eval set --${opts}

# extract input
while [[ $# -gt 0 ]]; do
    case "$1" in
	--tag|-t)
	        TAG=$2
		    shift 2
		        ;;
	--debug|-d)
	    DEBUG=1
	        shift 1
		    ;;
	--help|-h)
	    usage
	        shift 1
		    exit 0
		        ;;
	*)
	    break
	        ;;
    esac
done

if [[ -z ${TAG} ]]; then
    TAG=$(${GIT} describe --tags --abbrev=0)
    echo "No tag specified using last git tag: ${TAG}"
fi

# create directory structure in tmp
DIRNAME="${PWD##*/}"
TMP=/tmp
TMP_ROOT=${TMP}/${DIRNAME}
${MKDIR} -p ${TMP_ROOT}/lib

# copy required files to the correct directories
${CP} -r lib/!(.DS_Store) ${TMP_ROOT}/lib
${CP} -r __init__.py LICENSE.md plugin.json README.md SpaceMouseTool.py ${TMP_ROOT}

# compress the folder
ZIPFILE="${DIRNAME}-${TAG}.zip"
TARGZFILE="${DIRNAME}-${TAG}.tar.gz"
${CD} ${TMP}
${ZIP} -rq "${ZIPFILE}" "${DIRNAME}"
${TAR} -zcf "${TARGZFILE}" "${DIRNAME}"
${CD} ~-

# remove temporary directory
rm -rf ${TMP_ROOT}

# get github acces token
TOKEN=$(git config github.token)

# get branch, user, and repo name
BRANCH=master
URL=$(git config --get remote.origin.url)
RE="^(https|git)(:\/\/|@)([^\/:]+)[\/:]([^\/:]+)\/(.+).git$"
if [[ ${URL} =~ ${RE} ]]; then
protocol=${BASH_REMATCH[1]}
separator=${BASH_REMATCH[2]}
hostname=${BASH_REMATCH[3]}
USER=${BASH_REMATCH[4]}
REPO=${BASH_REMATCH[5]}
fi

# create release on github
generate_post_data()
{
  cat <<EOF
{
  "tag_name": "${TAG}",
  "target_commitish": "${BRANCH}",
  "name": "${TAG}",
  "body": "",
  "draft": false,
  "prerelease": false
}
EOF
}

RESP=$(curl --data "$(generate_post_data)" -H "Authorization: token ${TOKEN}" https://api.github.com/repos/${USER}/${REPO}/releases)

debug ${RESP}

# extract release id:
RELEASE_ID=$(echo ${RESP} | python3.8 -c "import sys, json; print(json.load(sys.stdin)['id'])")

# upload asset files
upload_asset()
{
  RESP=$(curl \
          -H "Authorization: token ${TOKEN}" \
          -H "Content-Type: $(file -b --mime-type ${1})" \
          --data-binary @"${1}" \
          "https://uploads.github.com/repos/${USER}/${REPO}/releases/${RELEASE_ID}/assets?name=$(basename ${1})")
}

debug ${RESP}

upload_asset "${TMP}/${ZIPFILE}"
upload_asset "${TMP}/${TARGZFILE}"

rm -rf "${TMP}/${ZIPFILE}"
rm -rf "${TMP}/${TARGZFILE}"