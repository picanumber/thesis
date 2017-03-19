#!/usr/bin/python
# -*- coding: utf-8 -*-

"""
----------------------------------------------------------
Benchmark visualizer           
----------------------------------------------------------
Utility to plot the output produced by benchmark.h        
----------------------------------------------------------
author         : Nikos Athanasiou
last modified  : 21 / 01 / 2017
website        : https://ngathanasiou.wordpress.com/ 
----------------------------------------------------------
"""
                        
import os
import ast
import pickle
import numpy as np
import pylab as pl
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
from itertools import cycle
import matplotlib.patches as mpatches
from matplotlib.patches import Polygon
import pandas as pd
from tkinter import *
import tkinter.filedialog as tkFileDialog

# -----------------------------------------------------------------------   
def get_numeric_list(any_list):
    """ Gets a list of numbers out of a list of strings 
    (+ mag specifiers) or a list of numbers (ONLY ints)
    """ 
    ret_list = []
    for elem in any_list:
        if isinstance(elem, str):          
            num = int(filter(str.isdigit, elem))
            if 'k' in elem:   num *= 1e03
            elif 'M' in elem: num *= 1e06
            elif 'G' in elem: num *= 1e09
            ret_list.append(num)
        else: ret_list.append(elem)

    return ret_list
# -----------------------------------------------------------------------

# -----------------------------------------------------------------------   
def get_description_list(any_list):
    """ Gets a list of strings out of a list of numbers or strings
    """ 
    ret_list = []
    for elem in any_list:
        if isinstance(elem, int):          
            if elem >= 1000000: val = '{}M'.format(elem / 1000000.)
            elif elem >= 1000: val = '{}K'.format(elem / 1000.)
            else: val = '{}'.format(elem)
            ret_list.append(val)
        else: ret_list.append(elem)

    return ret_list
# -----------------------------------------------------------------------            

# -----------------------------------------------------------------------            
def plot_simple_bmks(simples):
    """ All benchmarks (dictonaries) printed in the same graph
    """
    if len(simples):
        plt.clf()
        data = []
        labl = []
        
        for bmk in simples:
            labl.append(bmk['experiment_name'])
            data. append(np.array(bmk['timings']))

        plt.title(simples[0]['benchmark_name']) 
        plt.boxplot(data)

        plt.ylabel(bmk['time_type'])
        plt.xticks(range(1, 1+len(simples)), labl)
        
        plt.show()
# -----------------------------------------------------------------------            

# -----------------------------------------------------------------------            
def plot_factored_benchmark(bmk, col, syb):
    """ bmk is a dictionary
    """
    xdata = get_numeric_list(bmk['factors'])
    xlegend = get_description_list(bmk['factors'])
    ydata = bmk['timings']

    plt.xlabel(bmk['factor_name'])
    plt.ylabel(bmk['time_type'])

    plt.plot(xdata, ydata, syb, color=col, label=bmk['experiment_name'])
    #plt.semilogx(xdata, ydata, syb, color=col, label=bmk['experiment_name'])

    #plt.xticks(xdata, bmk['factors'])
    plt.xticks(xdata, xlegend)
    plt.xlim(min(xdata), max(xdata))
    
def plot_factored_bmks(factored):
    """ Each benchmark will have its own plot curve
    """
    plt.clf()
    cols = ['r', 'g', 'b', 'c', 'y', 'k', 'pink', 'gray']
    sybs = ['-o', '-s', '-*', '-+', '-v', '-x', '--o', '--s', '-D']

    if len(factored): plt.title(factored[0]['benchmark_name']) 
    col = cycle(cols)
    syb = cycle(sybs)
    for bmk in factored:
        plot_factored_benchmark(bmk, next(col), next(syb))

    legend = plt.legend(loc='upper left', shadow=True)
    #legend.get_frame().set_facecolor('#00FFCC') 
    plt.show()
# -----------------------------------------------------------------------            

# -----------------------------------------------------------------------            
def plot_benchmark_dicts(bmk_dict_list):
    """ Plot the list of benchmark dictionaries
    """
    factored = [x for x in bmk_dict_list if 'factors' in x]
    simples  = [x for x in bmk_dict_list if 'factors' not in x]
    if len(factored): plot_factored_bmks(factored)
    if len(simples): plot_simple_bmks(simples)
# -----------------------------------------------------------------------            

# -----------------------------------------------------------------------            
def plot_benchmark_file(filename):
    """ Plot the benchmark file
    """
    with open(filename) as f:
        content   = f.readlines() # every line is a dictionary
        all_dicts = [ast.literal_eval(line) for line in content]
        plot_benchmark_dicts(all_dicts)
# -----------------------------------------------------------------------            

# -----------------------------------------------------------------------            
def openwindows():
    nam = tkFileDialog.askopenfilename()
    if nam != '':
        plot_benchmark_file(nam)
# -----------------------------------------------------------------------            

# -----------------------------------------------------------------------            
def scatter_3d(fname, xs_min, ys_max, zs_lat): 
    """
    """
    plt.clf()
    fig = plt.figure()
    ax = fig.add_subplot(111, projection='3d')

    for xs, ys, zs in zip(xs_min, ys_max, zs_lat):
        ax.scatter(xs, ys, zs)

    ax.set_xlim((min(xs_min), max(xs_min)))
    ax.set_ylim((min(ys_max), max(ys_max)))
    ax.set_zlim((min(zs_lat), max(zs_lat)))
    ax.set_xlabel('min capacity')
    ax.set_ylabel('max capacity')
    ax.set_zlabel('latency (ms)')

    titolo = fname.split('/')
    plt.title(titolo[-1].rstrip('.txt')) 
    plt.show()
# -----------------------------------------------------------------------            

# -----------------------------------------------------------------------            
def plot_search_space(fname): 
    """
    """
    with open(fname, 'rb') as handle:
        xs_min = []
        ys_max = []
        zs_lat = []
        ss_dict = pickle.loads(handle.read())
        for elem in ss_dict: 
            (low, high) = elem.split(':')
            xs_min.append(int(low))
            ys_max.append(int(high))
            zs_lat.append(ss_dict[elem])

        scatter_3d(fname, xs_min, ys_max, zs_lat)
        
# -----------------------------------------------------------------------            

# -----------------------------------------------------------------------            
def visualize_search_space(): 
    nam = tkFileDialog.askopenfilename()
    if nam != '':
        plot_search_space(nam)
# -----------------------------------------------------------------------            

# -----------------------------------------------------------------------            
def plot_benchmarks():
    root = Tk()
    myfiletypes = [('Python files', '*.py'), ('All files', '*')]
    open = tkFileDialog.Open(root, filetypes = myfiletypes)
    Button(root, text="PLOT Benchmark", command=openwindows).pack()
    Button(root, text="Search Space", command=visualize_search_space).pack()

    statusbar = Label(root, text="", bd=1, relief=SUNKEN, anchor=W)
    statusbar.pack(side=BOTTOM, fill=X)
    root.mainloop()
# -----------------------------------------------------------------------            

# -----------------------------------------------------------------------            
def main(): 
    plot_benchmarks()
# -----------------------------------------------------------------------            

# -----------------------------------------------------------------------            
if __name__ == '__main__': 
    main()
# -----------------------------------------------------------------------            

