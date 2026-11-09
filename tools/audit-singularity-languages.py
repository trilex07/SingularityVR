#!/usr/bin/env python3
"""Inspect owned GOG language_setup.ini and archive listing without executing it."""
import argparse
import configparser
import hashlib
import json
import re
from pathlib import Path


def audit(root, listing):
    config = configparser.ConfigParser(interpolation=None)
    config.read(root/'language_setup.ini', encoding='utf-8-sig')
    codes = dict(English='INT', German='DEU', French='FRA', Spanish='ESN', Italian='ITA')
    paths = [line[7:].replace('\\', '/') for line in listing.read_text(encoding='utf-8-sig').splitlines()
             if line.startswith('Path = game\\')]
    rows = []
    for language in config['Options'].values():
        section = config[language]
        relative = section['LocalizedFileName'].removeprefix('{app}/')
        path = (root/relative).resolve()
        if not path.is_relative_to(root.resolve()):
            raise ValueError('language executable outside supplied root')
        data = path.read_bytes()
        code = codes.get(language)
        packages = [p for p in paths if code and re.search('_LOC_'+code+r'\.xxx$', p, re.I)]
        movies = [p for p in paths if code and '/Movies/' in p and re.search('_'+code+r'\.bik$', p, re.I)]
        rows.append(dict(language=language, executable=relative, bytes=len(data),
                         sha256=hashlib.sha256(data).hexdigest().upper(),
                         configuredLanguageValue=int(section['Value']),
                         localizedPackages=len(packages), localizedMovies=movies))
    return dict(execution='NOT RUN; setup/registry values inspected only', languages=rows,
                distinctExecutables=len({r['sha256'] for r in rows}),
                limitation='Shared executable proves shared native code, not localized asset/runtime acceptance.')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('game_root', type=Path)
    parser.add_argument('archive_listing', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    result = audit(args.game_root, args.archive_listing)
    args.output.write_text(json.dumps(result, indent=2)+'\n', encoding='utf-8')
    for row in result['languages']:
        print(row['language'], row['executable'], f"{row['localizedPackages']} localized packages", row['sha256'])
    print('Distinct executable hashes:', result['distinctExecutables'])


if __name__ == '__main__':
    main()
