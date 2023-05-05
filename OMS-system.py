import numpy as np
import sys
from tqdm import tqdm
from PIL import Image, ImageDraw


number = 1
startvalue = 3
endvalue = 2






vertices = np.array([[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
                    [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,0,0],
                    [0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
                    [0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0],
                    [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0],
                    [0,0,1,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0],
                    [0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0],
                    [0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
                    [0,0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0],
                    [0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
                    [0,0,0,1,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0],
                    [0,0,0,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0],
                    [0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
                    [0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
                    [0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
                    [0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
                    [0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
                    [0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
                    [0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
                    [3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0]])


"""
vertices = np.array(
           [[0,0,0,1,0,2,0],
            [0,1,0,0,0,0,1],
            [0,0,0,0,0,0,0],
            [0,1,0,1,0,1,1],
            [0,1,0,0,0,0,0],
            [1,0,0,0,0,0,0],
            [3,0,0,0,0,0,0]])
"""


np.fliplr(vertices)


num_of_source_edge = 2


Ye,Xe = (np.where((vertices==endvalue)))


zippede = np.column_stack((Xe,Ye))


end = [zippede[0][0],zippede[0][1]]


Y,X = (np.where((vertices==number)))


Yn = np.append(Y,Ye)
Xn = np.append(X,Xe)


zipped = np.column_stack((Xn,Yn))


Ys,Xs = (np.where((vertices==startvalue)))


zippedS = np.column_stack((Xs,Ys))
route1,route2,route3 = [],[],[]
routes = [route1,route2,route3]


next_val = []
road = [[zippedS[0][0],zippedS[0][1]]]
oldroad = []
def messurevalue(road_list,values_list,length_parameter): #måler lengden mellom to punkt
   length = np.sqrt((abs(road_list[len(length_parameter)-1][0]-values_list[0]))**2+(abs(road_list[len(length_parameter)-1][1]-values_list[1]))**2)
   return length


def checkifnotinlist(road_vals,test_vals): #sjekker at et element ikke er i en liste
   boolean = True
   for i in road_vals:
       if(i[0] == test_vals[0] and i[1] == test_vals[1]):
           boolean = False
   return boolean


def findroute(oldroad): #finner en rute med å se på nærmeste punkt som ikke allerede er valgt
   roadlength = 0
   for t in tqdm(range(len(zipped))):
       lowest = []
       low_val = 100
       for i in zipped:   
           if (checkifnotinlist(road,i)):
               if(checkifnotinlist(oldroad,i)):
                   test_val = messurevalue(road,i,road)
                   if(test_val<low_val):
                       low_val = test_val
                       if([i[0],i[1]] == end):
                           road.append(end)
                           roadlength +=low_val
                           return road,roadlength
                       lowest = [i[0],i[1]]
       roadlength +=low_val
       road.append(lowest)


def GetOldRoad():
   low_val = 100
   road = [[zippedS[0][0],zippedS[0][1]]]
   for i in zipped:   
       if (checkifnotinlist(road,i)):
           if(checkifnotinlist(oldroad,i)):
               test_val = messurevalue(road,i,road)
               if(test_val<low_val):
                   low_val = test_val
                   lowest = [i[0],i[1]]
   lowest1 =[lowest]
   low_val = 5
   if low_val > 4:
       low_val = 100
       for i in oldroad:   
           test_val = messurevalue(lowest1,i,lowest1)
           if(test_val<low_val):
               low_val = test_val
               lowest = [i[0],i[1]]
       for val in range(route1.index(lowest)):
           oldroad.pop(val)


def checkifallroutesfound(): #sjekker om alle mulige ruter er funnet ved å se at alle mulige kordinater er brukt
   checklist = []
   for i in routes:
       for element in i:
           if i not in checklist:
               checklist.append(i)
               print("hei")
  
   print(len(checklist))
   return len(zipped)-len(checklist)


def FindBestRoute(): #finner beste rute ved å sammenligne alle mulige ruter
   print(1)


route1,length1 = findroute(oldroad)
#print(route1,length1)
oldroad = route1[:-1]
GetOldRoad()
road = [[zippedS[0][0],zippedS[0][1]]]
route2,length2 = findroute(oldroad)
#print(route2,length2)
print(routes[0])
print(checkifallroutesfound())


im = Image.new('RGB', (len(vertices)+1, len(vertices)+1), (0, 0, 0))
draw = ImageDraw.Draw(im)


draw_lin1 = ()
draw_lin2 = ()


for i in route1:
   draw_lin1 +=(i[0],i[1])


for i in route2:
   draw_lin2 +=(i[0],i[1])


draw.line((draw_lin1), fill=(255, 255, 0))
draw.line((draw_lin2), fill=(0, 255, 255))
draw.point((end[0],end[1]), fill=(255, 255, 255))


im.show()
