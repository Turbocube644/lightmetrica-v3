# ---
# jupyter:
#   jupytext:
#     formats: ipynb,py:light
#     text_representation:
#       extension: .py
#       format_name: light
#       format_version: '1.3'
#       jupytext_version: 1.0.1
#   kernelspec:
#     display_name: Python 3
#     language: python
#     name: python3
# ---

# ## Rendering all scenes
#
# This test checks scene loading and basic rendering for all test scenes.

# %load_ext autoreload
# %autoreload 2

import os
import imageio
import pandas as pd
import numpy as np
# %matplotlib inline
import matplotlib.pyplot as plt
from mpl_toolkits.axes_grid1 import make_axes_locatable
import lmfunctest as ft
import lmscene
import lightmetrica as lm

os.getpid()

# %load_ext lightmetrica_jupyter

lm.init('user::default', {})
lm.parallel.init('parallel::openmp', {
    'numThreads': -1
})
lm.log.init('logger::jupyter', {})
lm.info()

lm.comp.detail.loadPlugin(os.path.join(ft.env.bin_path, 'accel_nanort'))
lm.comp.detail.loadPlugin(os.path.join(ft.env.bin_path, 'objloader_tinyobjloader'))

lm.objloader.init('objloader::tinyobjloader', {})

for scene in lmscene.scenes():
    lm.reset()
    
    # Film
    lm.asset('film_output', 'film::bitmap', {
        'w': 1920,
        'h': 1080
    })
    
    # Render
    lmscene.load(ft.env.scene_path, scene)
    lm.build('accel::nanort', {})
    lm.render('renderer::raycast', {
        'output': lm.asset('film_output'),
        #'use_constant_color': True
    })
    img = np.flip(np.copy(lm.buffer(lm.asset('film_output'))), axis=0)
    
    # Visualize
    f = plt.figure(figsize=(15,15))
    ax = f.add_subplot(111)
    ax.imshow(np.clip(np.power(img,1/2.2),0,1))
    ax.set_title(scene)
    plt.show()

