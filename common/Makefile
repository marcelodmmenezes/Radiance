includes = -I$(TP) -I$(TP)/glm -I$(TP)/imgui

TP = ../thirdParty
objects = baseApp.o flyThroughCamera.o glContext.o objParser.o texture.o renderbuffer.o framebuffer.o

all: $(objects)

$(objects): %.o: %.hpp %.cpp
	g++ -c $(*F).cpp -o $@ $(includes)

