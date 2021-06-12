import sqlite3
import time
import logging
import os
import zipfile
import re
import json

def init():
    """
    Create a build table if not existing
    """
    with sqlite3.connect('ota_database.db') as connect:
        cursor = connect.cursor()
        cursor.execute("""
        CREATE TABLE if not exists Builds (
            FileName TEXT,
            UploadTime INTEGER,
            Path TEXT,
            BuildID TEXT,
            BuildVersion TEXT,
            BuildFlavor TEXT,
            Partitions TEXT
        )
        """)

def build_analyse(path, build_info):
    """
    Analyse the build's version info and partitions included
    Then write them into the build_info
    Args:
        path: the path of the build file
        build_info: a dict holds the info
    """
    def extract_info(pattern, lines):
        # Try to match a regex in a list of string
        line = list(filter(pattern.search, lines))[0]
        if line:
            return pattern.search(line).group(0)
        else:
            return ''
            
    build = zipfile.ZipFile(path)
    try:
        with build.open('SYSTEM/build.prop', 'r') as build_prop:
            raw_info = build_prop.readlines()
            pattern_id = re.compile(b'(?<=ro\.build\.id\=).+')
            pattern_version = re.compile(b'(?<=ro\.build\.version\.incremental\=).+')
            pattern_flavor = re.compile(b'(?<=ro\.build\.flavor\=).+')
            build_info['build_id'] = extract_info(pattern_id, raw_info).decode('utf-8')
            build_info['build_version'] = extract_info(pattern_version, raw_info).decode('utf-8')
            build_info['build_flavor'] = extract_info(pattern_flavor, raw_info).decode('utf-8')
    except KeyError:
        build_info['build_id'] = ''
        build_info['build_version'] = ''
        build_info['build_flavor'] = ''
    try:
        with build.open('META/ab_partitions.txt', 'r') as partition_info:
            raw_info = partition_info.readlines()
            partitions = []
            for line in raw_info:
                partitions.append(line.decode('utf-8').rstrip())
            build_info['partitions'] = json.dumps(partitions)
    except KeyError:
        build_info['partitions'] = '[]'

def new_build(filename, path):
    """
    Insert a new build into the database
    Args:
        filename: the name of the file
        path: the relative path of the file 
    """
    build_info = {}
    build_info['filename'] = filename
    build_info['path'] = path
    build_info['time'] = int(time.time())
    build_analyse(path, build_info)
    with sqlite3.connect('ota_database.db') as connect:
        cursor = connect.cursor()
        cursor.execute("""
        SELECT * FROM Builds WHERE FileName=:filename and Path=:path
        """, build_info)
        if cursor.fetchall():
            cursor.execute("""
            DELETE FROM Builds WHERE FileName=:filename and Path=:path
            """, build_info)
        cursor.execute("""
        INSERT INTO Builds (FileName, UploadTime, Path, BuildID, BuildVersion, BuildFlavor, Partitions)
        VALUES (:filename, :time, :path, :build_id, :build_version, :build_flavor, :partitions)
        """, build_info)

def new_build_from_dir(path):
    """
    Update the database using files under a directory
    Args:
        path: a directory
    """
    builds_name = os.listdir(path)
    for build_name in builds_name:
        new_build(build_name, os.path.join(path, build_name))

def get_builds():
    """
    Get a list of builds in the database
    Return:
        A list of build_info, each of which is a tuple:
        (FileName, UploadTime, Path, Build ID, Build Version, Build Flavor, Partitions)  
    """
    def sql_to_dict(row):
        target_info = {}
        target_info['FileName'] = row[0]
        target_info['UploadTime'] = row[1]
        target_info['Path'] = row[2]
        target_info['BuildID'] = row[3]
        target_info['BuildVersion'] = row[4]
        target_info['BuildFlavor'] = row[5]
        target_info['Partitions'] = row[6]
        return target_info

    with sqlite3.connect('ota_database.db') as connect:
        cursor = connect.cursor()
        cursor.execute("""
        SELECT FileName, UploadTime, Path, BuildID, BuildVersion, BuildFlavor, Partitions 
        FROM Builds""")
        return list(map(sql_to_dict, cursor.fetchall()))