#!/usr/bin/env python
import argparse
import re

import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt

parser = argparse.ArgumentParser()
parser.add_argument('files', type=str, nargs='+', help="log files")
parser.add_argument('--frequency_ghz', type=float, default=0.666)

args = parser.parse_args()

class LogData(object):
    def __init__(self, logfiles):
        if isinstance(logfiles, str):
            self.logfiles = { self._name_from_file(logfiles) : logfiles }
        elif isinstance(logfiles, dict):
            self.logfiles = logfiles
        else:
            names = [self._name_from_file(x) for x in logfiles]
            self.logfiles = dict(zip(names, logfiles))

        self.dataframe = None
        self.vars = set()

        self.parse()


    def _name_from_file(self, filename):
        return filename.split('_')[-1].split('.')[0]


    def _parse_line(self, line, data):
        complete = False

        match = re.search(r"(\d+)KB.*: (\d+) \| (\d+) \| (\d+)", line)

        if match:
            # last line
            kb   = int(match.group(1))
            res1 = int(match.group(2))
            res2 = int(match.group(3))
            res3 = int(match.group(4))
            if line.find('Cycles') > 0:
                res1 = res1 / args.frequency_ghz
                res2 = res2 / args.frequency_ghz
                res3 = res3 / args.frequency_ghz

            data[0]['KB'] = kb
            data[1]['KB'] = kb
            data[2]['KB'] = kb
            data[0]['nsec/KB'] = res1
            data[1]['nsec/KB'] = res2
            data[2]['nsec/KB'] = res3

            complete = True

        else:
            match = re.search(r"([\w\s]+): (\d+)", line)
            if match:
                what  = match.group(1).strip()
                value = int(match.group(2))
                if what in data[-1]:
                    data.append(dict())
                data[-1][what] = value
                self.vars.add(what)

        return complete


    def parse(self):
        data = list()
        for name, file in self.logfiles.items():
            rows = [dict()]
            with open(file, 'r') as f:
                for line in f:
                    if self._parse_line(line.strip(), rows):
                        i = 1
                        for row in rows:
                            row['target'] = "%s-%d" % (name, i)
                            i += 1
                            data.append(row)
                        rows = [dict()]

        self.dataframes = pd.DataFrame(data)


    def targets(self):
        return sorted(set(self.dataframes["target"].values))


if __name__ == "__main__":
    data = LogData(args.files)

    df = data.dataframes
    print(df)

    targets = data.targets()

    fig, axes = plt.subplots(5, len(targets), sharex=True, sharey="row")

    column = 0
    for t in targets:
        axes[0][column].set_title(t)
        axes[0][column].set_xscale('log')

        d = df.query("target == '%s'" % t)

        axes[0][column].plot(d["KB"].values, d["nsec/KB"].values, marker='o');

        for var, ax in zip(sorted(data.vars), axes[1:]):
            ax[column].plot(d['KB'].values, d[var].values, marker='o')
            if column == 0:
                ax[column].set_ylabel(var)

        column += 1


    plt.show()
