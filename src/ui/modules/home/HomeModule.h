#pragma once

// TODO: Could probably make a module class

namespace Modex
{
	class HomeModule
	{
	public:
		HomeModule();
		~HomeModule() = default;
		
		HomeModule(const HomeModule&) = delete;
		HomeModule(HomeModule&&) = delete;
		HomeModule& operator=(const HomeModule&) = delete;
		HomeModule& operator=(HomeModule&&) = delete;
		
		static void Draw();
		static void Init();
	private:

	};
}