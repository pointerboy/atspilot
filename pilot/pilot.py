import numpy as np
from PIL import ImageGrab

import cv2
import time
from pilot.input.d3_directkeys import ReleaseKey, PressKey, Keys

last_time = time.time()

def process_img(original):
    new_img = cv2.cvtColor(original, cv2.COLOR_BGR2GRAY)
    new_img = cv2.Canny(new_img, threshold1=200, threshold2=300)
    return new_img

while(True):
    screen = np.array(ImageGrab.grab(bbox=(0, 40, 800, 640)))
    new_screen = process_img(screen)

    print('Look took {}'.format(time.time()-last_time))
    last_time = time.time() # 123
    cv2.imshow('window', new_screen)
    if cv2.waitKey(25) & 0xFF == ord('q'):
        cv2.destroyAllWindows()
        break