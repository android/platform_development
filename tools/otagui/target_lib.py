from dataclasses import dataclass, asdict
import sqlite3
import time
import logging
import os
import zipfile
import re
import json


@dataclass
class BuildInfo:
    """
    A class for Android build information.
    Args:
        in_database: True if you don't want to re-parse the build prop
    """
    in_database: bool
    file_name: str
    path: str
    time: int
    build_id: str = ''
    build_version: str = ''
    build_flavor: str = ''
    partitions: str = ''

    def __post_init__(self):
        """
        Analyse the build's version info and partitions included
        Then write them into the build_info
        """
        if self.in_database:
            return

        def extract_info(pattern, lines):
            # Try to match a regex in a list of string
            line = list(filter(pattern.search, lines))[0]
            if line:
                return pattern.search(line).group(0)
            else:
                return ''

        build = zipfile.ZipFile(self.path)
        try:
            with build.open('SYSTEM/build.prop', 'r') as build_prop:
                raw_info = build_prop.readlines()
                pattern_id = re.compile(b'(?<=ro\.build\.id\=).+')
                pattern_version = re.compile(
                    b'(?<=ro\.build\.version\.incremental\=).+')
                pattern_flavor = re.compile(b'(?<=ro\.build\.flavor\=).+')
                self.build_id = extract_info(
                    pattern_id, raw_info).decode('utf-8')
                self.build_version = extract_info(
                    pattern_version, raw_info).decode('utf-8')
                self.build_flavor = extract_info(
                    pattern_flavor, raw_info).decode('utf-8')
        except KeyError:
            pass
        try:
            with build.open('META/ab_partitions.txt', 'r') as partition_info:
                raw_info = partition_info.readlines()
                for line in raw_info:
                    self.partitions += line.decode('utf-8').rstrip() + ','
        except KeyError:
            pass


class TargetLib:
    def __init__(self, path='ota_database.db'):
        """
        Create a build table if not existing
        """
        self.path = path
        with sqlite3.connect(self.path) as connect:
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

    def new_build(self, filename, path):
        """
        Insert a new build into the database
        Args:
            filename: the name of the file
            path: the relative path of the file
        """
        build_info = BuildInfo(False, filename, path, int(time.time()))
        logging.info(asdict(build_info))
        with sqlite3.connect(self.path) as connect:
            cursor = connect.cursor()
            cursor.execute("""
            SELECT * FROM Builds WHERE FileName=:file_name and Path=:path
            """, asdict(build_info))
            if cursor.fetchall():
                cursor.execute("""
                DELETE FROM Builds WHERE FileName=:file_name and Path=:path
                """, asdict(build_info))
            cursor.execute("""
            INSERT INTO Builds (FileName, UploadTime, Path, BuildID, BuildVersion, BuildFlavor, Partitions)
            VALUES (:file_name, :time, :path, :build_id, :build_version, :build_flavor, :partitions)
            """, asdict(build_info))

    def new_build_from_dir(self, path):
        """
        Update the database using files under a directory
        Args:
            path: a directory
        """
        if os.path.isdir(path):
            builds_name = os.listdir(path)
            for build_name in builds_name:
                self.new_build(build_name, os.path.join(path, build_name))
        elif os.path.isfile(path):
            self.new_build(os.path.split(path)[-1], path)
        return self.get_builds()

    def sql_to_dict(self, row):
        build_info = BuildInfo(
            True, *row)
        build_info_dict = asdict(build_info)
        build_info_dict['partitions'] = build_info_dict['partitions'].split(',')[
            :-1]
        build_info_dict.pop('in_database', None)
        return build_info_dict

    def get_builds(self):
        """
        Get a list of builds in the database
        Return:
            A list of build_info, each of which is an object:
            (FileName, UploadTime, Path, Build ID, Build Version, Build Flavor, Partitions)
        """
        with sqlite3.connect(self.path) as connect:
            cursor = connect.cursor()
            cursor.execute("""
            SELECT FileName, Path, UploadTime, BuildID, BuildVersion, BuildFlavor, Partitions
            FROM Builds""")
            return list(map(self.sql_to_dict, cursor.fetchall()))

    def get_builds_by_path(self, path):
        """
        Get a build in the database by its path
        Return:
            A build_info, which is an object:
            (FileName, UploadTime, Path, Build ID, Build Version, Build Flavor, Partitions)
        """
        with sqlite3.connect(self.path) as connect:
            cursor = connect.cursor()
            cursor.execute("""
            SELECT FileName, Path, UploadTime, BuildID, BuildVersion, BuildFlavor, Partitions
            WHERE Path==(?)
            """, (path, ))
        return self.sql_to_dict(cursor.fetchone())
