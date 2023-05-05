import numpy as np
from skimage.morphology import skeletonize
from PIL import Image
import math

# img_test = np.array([
#                     [0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0],
#                     [0,0,0,1,1,1,1,1,1,1,1,0,1,0,1,0,0,2,0,0],
#                     [0,0,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0],
#                     [0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0],
#                     [1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0],
#                     [1,1,1,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0],
#                     [0,0,1,1,1,0,0,0,0,0,0,0,1,0,1,0,0,0,0,0],
#                     [0,0,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
#                     [0,0,1,1,1,1,0,0,0,0,1,0,0,0,0,0,0,0,0,0],
#                     [0,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
#                     [0,0,0,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0],
#                     [0,0,0,0,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0],
#                     [0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
#                     [0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
#                     [0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
#                     [0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
#                     [0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
#                     [0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
#                     [1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
#                     [3,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0]])

h, w = 400, 400
img_test = np.zeros((h, w))
for y, row in enumerate(img_test):
    for x, value in enumerate(row):
        if 0.5*y<x<0.5*y+20:
            img_test[y][x] = 1
        if 44<y<44+20:
            img_test[y][x] = 1

# img_test = np.array([
#     [0,0,0],
#     [0,1,0],
#     [0,0,0]
# ])

def is_background(img, y, x, background):
    if(0 < y < np.shape(img)[0] and 0 < x < np.shape(img)[1]):
        if(img[y][x] <= background):
            return True
        else:
            return False
    else:
        return True

def create_weight_img(img, background):
    background_found = False
    weight = np.shape(img)[0]
    weight_img = np.zeros(np.shape(img))
    for y, row in enumerate(img):
        for x, value in enumerate(row):
            n = 0
            background_found = False
            while (not background_found):
                for y_scan in range(y-n, y+n+1):
                    if(y_scan == y+n or y_scan == y-n):
                        for x_scan in range(x-n, x+n+1):
                            if(is_background(img, y_scan, x_scan, background)):
                                weight = min(math.dist((x, y), (x_scan, y_scan)), weight)
                                background_found = True
                    else:
                        x_scan = x-n
                        if(is_background(img, y_scan, x_scan, background)):
                            weight = min(math.dist((x, y), (x_scan, y_scan)), weight)
                            background_found = True
                        x_scan = x+n
                        if(is_background(img, y_scan, x_scan, background)):
                            weight = min(math.dist((x, y), (x_scan, y_scan)), weight)
                            background_found = True
                n += 1
            weight_img[y][x] = weight
            weight = 1000
    return weight_img

def threshold(weight_img, contrast):
    threshold = np.amax(weight_img) * contrast
    horse = np.copy(weight_img)
    for y, row in enumerate(weight_img):
        for x, weight in enumerate(row):
            horse[y, x] = 0
            if weight >= threshold:
                horse[y, x] = 1
    return horse

def create_horse(threshold_img):
    horse = skeletonize(threshold_img)
    return horse

def find_crossings(horse):
    crossings = np.zeros(np.shape(horse))
    for y, row in enumerate(horse):
        for x, value in enumerate(row):
            high_pixels = 0
            for y_scan in range(y-1, y+2):
                if(y_scan == y+1 or y_scan == y-1):
                        for x_scan in range(x-1, x+2):
                            if(0 < y_scan < np.shape(horse)[0] and 0 < x_scan < np.shape(horse)[1]):
                                if(horse[y_scan][x_scan] > 0):
                                    high_pixels += 1
                else:
                    x_scan = x-1
                    if(0 < y_scan < np.shape(horse)[0] and 0 < x_scan < np.shape(horse)[1]):
                        if(horse[y_scan][x_scan] > 0):
                            high_pixels += 1
                    x_scan = x+1
                    if(0 < y_scan < np.shape(horse)[0] and 0 < x_scan < np.shape(horse)[1]):
                        if(horse[y_scan][x_scan] > 0):
                            high_pixels += 1
            if(high_pixels >= 3 and horse[y][x] > 0):
                crossings[y][x] = 1
    return crossings

def horse_convert(img, background):
    weight_img = create_weight_img(img, background)
    threshold_img = threshold(weight_img, 0.6)
    horse = create_horse(threshold_img)
    crossings_img = find_crossings(horse)
    return crossings_img


horse = horse_convert(img_test, 0.5)

max=np.amax(horse)
print(max)
blank_img = np.array([[[255*value/max, 255*value/max, 255*value/max] for value in row] for row in horse], dtype=np.uint8)

show_img = Image.fromarray(blank_img, 'RGB')
show_img.show()