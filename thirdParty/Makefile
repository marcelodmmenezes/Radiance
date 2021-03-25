includes = -I. -Iimgui

glad_objects = glad/glad.o
imgui_objects = imgui/imgui.o imgui/imgui_draw.o imgui/imgui_widgets.o
imgui_impl_objects = imgui/examples/imgui_impl_glfw.o imgui/examples/imgui_impl_opengl3.o

all: $(glad_objects) $(imgui_objects) $(imgui_impl_objects)

$(glad_objects): %.o: %.h %.c
	g++ -c $(*D)/$(*F).c -o $@

$(imgui_objects): %.o: imgui/imgui.h %.cpp
	g++ -c $(*D)/$(*F).cpp -o $@

$(imgui_impl_objects): %.o: imgui/imgui.h %.h %.cpp
	g++ -c $(basename $@).cpp -o $@ $(includes)

