
#include "VulkanBase.h"
#include "ParticleSystem.h"

int main() {
	ParticleSystem p;

	p.PrepareParticleSystem();

	while (!glfwWindowShouldClose(p.ReturnWindowHandle())) {
		glfwPollEvents();
		//p.Draw();
	}
	return 0;
}