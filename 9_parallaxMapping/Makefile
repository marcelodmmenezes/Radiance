TP = ../thirdParty
COMMON = ../common

includes = -I$(TP) -I$(TP)/glm -I$(TP)/imgui
flags = -lglfw3 -lgdi32 -lopengl32 -Wall -Wextra

glad_objects = $(TP)/glad/glad.o
imgui_objects = $(TP)/imgui/imgui.o $(TP)/imgui/imgui_draw.o $(TP)/imgui/imgui_widgets.o
imgui_impl_objects = $(TP)/imgui/examples/imgui_impl_glfw.o $(TP)/imgui/examples/imgui_impl_opengl3.o
common_objects = $(COMMON)/baseApp.o $(COMMON)/flyThroughCamera.o \
	$(COMMON)/glContext.o $(COMMON)/objParser.o \
	$(COMMON)/texture.o $(COMMON)/renderbuffer.o $(COMMON)/framebuffer.o

main: $(glad_objects) $(imgui_objects) $(imgui_impl_objects)
	g++ main.cpp \
	$(glad_objects) $(imgui_objects) $(imgui_impl_objects) \
	$(common_objects) \
	-o main.exe \
	$(includes) \
	$(flags)

