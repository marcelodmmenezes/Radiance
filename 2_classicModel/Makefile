TP = ../thirdParty/

main:
	g++ main.cpp \
	../common/baseApp.cpp \
	../common/glContext.cpp \
	../common/flyThroughCamera.cpp \
	../common/objParser.cpp \
	$(TP)glad/glad.c \
	$(TP)imgui/imgui.cpp $(TP)imgui/imgui_draw.cpp $(TP)imgui/imgui_widgets.cpp \
	$(TP)imgui/examples/imgui_impl_glfw.cpp $(TP)imgui/examples/imgui_impl_opengl3.cpp \
	-o main.out \
	-I$(TP) -I$(TP)glm -I$(TP)imgui -I/usr/local/include \
	-lglfw3 -ldl -pthread \
	-Wall -Wextra

