#!/usr/bin/python3

# Copyright (c) 2021, Institut National de Recherche en Informatique et en Automatique (Inria)
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
# * Redistributions of source code must retain the above copyright notice,
#   this list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
#
# * Neither the name of the copyright holder nor the names of its
#   contributors may be used to endorse or promote products derived from
#   this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
# IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
# PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
# TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
# NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import argparse
import glob
import matplotlib
import matplotlib.pyplot as plt
import matplotlib.animation as animation
import numpy as np
import os.path
import sys


def main():
    parser = argparse.ArgumentParser(
        description="Plot Melissa sensitivity analysis results as movies"
    )
    parser.add_argument("directory", help="Melissa output directory")
    parser.add_argument("field", help="The field or quantity to plot")
    parser.add_argument("statistic", help="The statistic to be plotted")
    parser.add_argument("-c", "--colormap", choices=plt.colormaps())
    parser.add_argument("--fps", type=int, help="movie frames per second")

    args = parser.parse_args()

    if not os.path.isdir(args.directory):
        return "{:s} is not a directory".format(args.directory)

    if args.fps is not None and args.fps <= 0:
        return "frames per second must be a positive integer, got {:d}".format(
            args.fps
        )

    cmap = args.colormap
    field = args.field
    statistic = args.statistic

    filename_pattern = os.path.join(
        args.directory, "results.{:s}_{:s}.*".format(field, statistic)
    )
    filenames = sorted(glob.glob(filename_pattern, recursive=False))

    print("Found {:d} files".format(len(filenames)))
    if len(filenames) == 0:
        print("Exiting.")
        return

    print("Reading files... This may take some time.")

    n_1 = 100
    n_2 = 100
    n_t = len(filenames)
    data = np.full([n_t, n_1, n_2], np.nan)
    for t, filename in enumerate(filenames):
        f = np.loadtxt(filename)
        data[t] = f.reshape(n_1, n_2)

    fig = plt.figure()
    ax = fig.add_subplot(111)

    print("Plotting data...")

    images = []
    for t in range(n_t):
        title_fmt = "{:s} {:s}: step {:d} of {:d}"
        title = title_fmt.format(field, statistic, t + 1, n_t)
        title_im = plt.text(
            0.5,
            1.0,
            title,
            horizontalalignment='center',
            verticalalignment='bottom',
            transform=ax.transAxes
        )
        data_im = plt.imshow(data[t], cmap=cmap)
        images.append([title_im, data_im])

    plt.colorbar()

    animation_duration_s = 10
    time_per_frame_ms = 1000 * animation_duration_s / n_t
    if args.fps is None:
        frames_per_second = n_t / animation_duration_s
    else:
        frames_per_second = args.fps

    anim = animation.ArtistAnimation(
        fig, images, interval=time_per_frame_ms, blit=False, repeat=True
    )
    writer = animation.FFMpegWriter(fps=frames_per_second)
    animation_path = os.path.join(
        args.directory, "{:s}-{:s}.mp4".format(field, statistic)
    )

    print("Encoding data as movie in {:s}...".format(animation_path))

    try:
        anim.save(animation_path, writer=writer)
    except FileNotFoundError as e:
        if e.filename == "ffmpeg":
            return "cannot save movie plot because ffmpeg cannot be found"
        raise e

    print("Showing plot")

    plt.show()


if __name__ == "__main__":
    sys.exit(main())
