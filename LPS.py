import numpy as np
import sys
from tqdm import tqdm
from PIL import Image, ImageDraw
import paho.mqtt.client as mqtt 
import time

mqttBroker ="mqtt.eclipseprojects.io" 

client = mqtt.Client("Angles")
client.connect(mqttBroker) 

number = 1
startvalue = 3
endvalue = 2
Calibrate_int = 0

vertices = np.array([[0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
                     [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
                     [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
                     [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
                     [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
                     [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
                     [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
                     [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
                     [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
                     [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
                     [0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0],
                     [1,1,1,1,1,1,1,0,1,0,1,0,0,1,0,1,0,2,0,0],
                     [0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
                     [0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
                     [0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
                     [0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
                     [0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
                     [0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
                     [0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
                     [3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0]])

np.fliplr(vertices)

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
Proad1,Proad2, Proad3 = [],[],[]
length1, length2, length3 = 0,0,0
routes = [route1,route2,route3]
previous_roads = [Proad1,Proad2,Proad3]
length_values = [length1,length2,length3]

next_val = []
road = [[zippedS[0][0],zippedS[0][1]]]
oldroad = [[]]



def messurevalue(road_list,values_list,length_parameter): #måler lengden mellom to punkt
    length = np.sqrt((abs(road_list[len(length_parameter)-1][0]-values_list[0]))**2+(abs(road_list[len(length_parameter)-1][1]-values_list[1]))**2)
    return length

def checkifnotinlist(road_vals,test_vals): #sjekker at et element ikke er i en liste
    if road_vals is None:
        return True
    for i in road_vals:
        if(i[0] == test_vals[0] and i[1] == test_vals[1]):
            return False
    return True


def findroute(oldroad,zipped,zippedS,end): #finner en rute med å se på nærmeste punkt som ikke allerede er valgt
    roadlength = 0
    road = [[zippedS[0][0],zippedS[0][1]]]
    for t in range(len(zipped)):
        lowest = []
        low_val = 1000
        for i in zipped:
            if (checkifnotinlist(road,i)):
                if(checkifnotinlist(oldroad,i)):
                    #if (NotInPrevoiusRoads(previous_roads,i)):
                        test_val = messurevalue(road,i,road)
                        if(test_val<low_val):
                            low_val = test_val
                            if([i[0],i[1]] == end):
                                road.append(end)
                                roadlength += low_val
                                return road,roadlength
                            lowest = [i[0],i[1]]
        
        if low_val >= 25: #Hvis man skal hoppe til sutten basert på hvor langt det er til neste punkt
            road.append(end)
            end_point = [end]
            if i[0] == end_point[0][0] and i[1] == end_point[0][1]:
                roadlength += low_val
                return road,roadlength
            length = messurevalue(end_point,i,end_point)
            roadlength +=length
            return road,roadlength
        roadlength +=low_val
        road.append(lowest)


def GetOldRoad(route_index,oldroad,zipped,zippedS,end): #Ser på den forrige ruten og gjør kunn de som må bli olovelige ulovelig
    low_val = 1000
    road = [[zippedS[0][0],zippedS[0][1]]]
    lowest = []
    for i in zipped: #finner den nærmeste lovlige
        if (checkifnotinlist(road,i) and [i[0],i[1]] != end):
            if(checkifnotinlist(oldroad,i)):
                test_val = messurevalue(road,i,road)
                if(test_val<low_val):
                    low_val = test_val
                    lowest = [i[0],i[1]]
                         
    lowest1 =[lowest]
    if low_val > 10: #Hvis den nærmeste er en viss lengde unna vet vi at vi må gjøre noe lovlig igjen
        low_val = 1000
        for i in oldroad: #Finner den nærmeste lovelige til ulovelige og finner det punktet på den ulovlige listen
            test_val = messurevalue(lowest1,i,lowest1)
            if(test_val<low_val):
                low_val = test_val
                lowest = [i[0],i[1]]
        
        index_num = oldroad.index(lowest)
        oldroad = oldroad[index_num+1:len(oldroad)+1] #Tar vekk alle punkt ifra starten til det punktet 
        previous_roads[route_index] = oldroad
        return oldroad


def checkifallroutesfound(routes,zipped): #sjekker om alle mulige ruter er funnet ved å se at alle mulige kordinater er brukt
    checklist = []
    for i in routes:
        for element in i:
            if element not in checklist:
                checklist.append(element)
    if (len(zipped)-len(checklist)+1) <= 0:
        return False
    if (len(zipped)-len(checklist)+1) > 0:
        return True

def GetAngle(first_point, second_point): #finner vinkelen i matrisen mellom to punkt
    angle = np.degrees(np.arctan2(second_point[0] - first_point[0], second_point[1] - first_point[1]))
    
    return angle

print(GetAngle([86, 146],[88, 130]))

def GetSmallestAngle(first_point, second_point, third_point):
    a = np.array([first_point[0], first_point[1]])
    b = np.array([second_point[0], second_point[1]])
    c = np.array([third_point[0], third_point[1]])
    
    ba = a - b
    bc = c - b
    
    cos_angle = np.dot(ba, bc)/(np.linalg.norm(ba) * np.linalg.norm(bc))
    angle = np.arccos(cos_angle)
    angle_deg = np.degrees(angle)
    return angle_deg


#kan iterere gjennom helt til alle punkt er brukt og gjøre alle nødvendige verdier lovlige til det er gjort

def FindBestRoute(zipped,zippedS,end): #finner beste rute ved å sammenligne alle mulige ruter
    index_val = 0
    routes = [route1,route2,route3]
    length_values = [length1,length2,length3]
    oldroad = [[zippedS[0][0],zippedS[0][1]]]
    print(len(routes))
    while((checkifallroutesfound(routes,zipped)) and index_val < len(routes)+1):
        print(index_val)
        routes[index_val], length_values[index_val] = findroute(oldroad,zipped,zippedS,end)
        if checkifallroutesfound(routes,zipped):
            oldroad = routes[index_val][:-1]  #problemet?
            oldroad = GetOldRoad(index_val,oldroad,zipped,zippedS,end)
        index_val +=1
        if index_val > 2:
            break
    min_index = length_values.index(min(length_values))
    start_val = 100
    if length_values[min_index] == 0:
        length_values.pop(min_index)
        min_index = length_values.index(min(length_values))
        return routes[min_index]
    return routes[min_index]
   
"""

bestroute = (FindBestRoute(zipped,zippedS,end))



im = Image.new('RGB', (len(vertices)+1, len(vertices)+1), (0, 0, 0))
draw = ImageDraw.Draw(im)

draw_lin1 = ()
draw_lin2 = ()
draw_lin3 = ()



for i in bestroute:
    draw_lin1 +=(i[0],i[1])

draw.line((draw_lin1), fill=(255, 255, 0))

draw.point((end[0],end[1]), fill=(255, 255, 255))

im.show()



def Calibrate_car():
    first_point = GetCarPoint()
    client.publish("State", 1)
    client.publish("Angles", 0)
    time.sleep(1)
    second_point = GetCarPoint()
    client.publish("State", 0)


def DriveRoute(route):
    point_index = 1
    current_point = route[point_index]
    condition = True
    while condition:
        client.publish("State", 1)
        if messurevalue(current_point,i,current_point) <=10:
            point_index +=1
            current_point = route[point_index]
        carpoint = GetCarPoint()
        angle = GetAngle(carpoint,current_point)
        client.publish("Angles", angle)
        if messurevalue(end,carpoint,end) <=4:
            condition = False
            client.publish("State", 0)


"""




