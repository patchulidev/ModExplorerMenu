#pragma once

#include "data/BaseObject.h"

namespace Modex
{
	class Data
	{
	public:

		enum PLUGIN_TYPE : uint32_t
		{
			ALL = 0,
			ITEM,
			NPC,
			OBJECT,
			CELL,
		};

		enum SORT_TYPE : uint32_t
		{
			ALPHABETICAL = 0,
			COMPILEINDEX_ASC,
			COMPILEINDEX_DESC,
		};

		struct ModFileItemFlags
		{
			bool 	alchemy 		= false;
			bool 	ingredient 		= false;
			bool 	ammo 			= false;
			bool 	key 			= false;
			bool 	misc 			= false;
			bool 	armor 			= false;
			bool 	book 			= false;
			bool 	weapon 			= false;
			bool 	scroll 			= false;
			bool 	npc 			= false;
			bool 	staticObject 	= false;
			bool 	tree 			= false;
			bool 	activator 		= false;
			bool 	container 		= false;
			bool 	door 			= false;
			bool 	light 			= false;
			bool 	furniture 		= false;
			bool	flora			= false;
			bool	cell			= false;
		};

		static inline Data* GetSingleton()
		{
			static Data singleton;
			return std::addressof(singleton);
		}

		void Run();
	
		std::unordered_set<const RE::TESFile*>			GetModulePluginList(PLUGIN_TYPE a_type);
		std::vector<const RE::TESFile*> 				GetModulePluginListSorted(PLUGIN_TYPE a_type, SORT_TYPE a_sortType);
		std::vector<std::string> 						GetSortedListOfPluginNames();
		std::vector<std::string> 						GetFilteredListOfPluginNames(PLUGIN_TYPE a_type, SORT_TYPE a_sort);
		bool											IsFormTypeInPlugin(const RE::TESFile* a_plugin, RE::FormType a_formType);
		void 											CacheNPCRefIds();
		void 											GenerateNPCClassList();
		void 											GenerateNPCRaceList();
		void 											GenerateNPCFactionList();
		void 											AddModToIndex(const RE::TESFile* a_mod, std::unordered_set<const RE::TESFile*>& a_out);

		[[nodiscard]] inline std::vector<BaseObject>& 	GetInventoryList()	{ return m_inventory; 		}
		[[nodiscard]] inline std::vector<BaseObject>& 	GetAddItemList() 	{ return m_cache; 			}
		[[nodiscard]] inline std::vector<BaseObject>& 	GetNPCList() 		{ return m_npcCache; 		}
		[[nodiscard]] inline std::vector<BaseObject>& 	GetObjectList() 	{ return m_staticCache;		}
		[[nodiscard]] inline std::vector<CellData>& 	GetTeleportList() 	{ return m_cellCache; 		}
		[[nodiscard]] inline std::set<std::string> 		GetNPCClassList() 	{ return m_npcClassList; 	}
		[[nodiscard]] inline std::set<std::string> 		GetNPCRaceList() 	{ return m_npcRaceList; 	}
		[[nodiscard]] inline std::set<std::string> 		GetNPCFactionList() { return m_npcFactionList; 	}
		[[nodiscard]] CellData& 						GetCellByEditorID(const std::string& a_editorid);

		void											GenerateInventoryList();
		void 											GenerateItemList();
		void 											GenerateNPCList();
		void 											GenerateObjectList();
		void 											GenerateCellList();


		void SortAddItemList();

	private:
		std::vector<BaseObject> 						m_inventory;
		std::vector<BaseObject> 						m_cache;
		std::vector<BaseObject> 						m_staticCache;
		std::vector<BaseObject> 						m_npcCache;
		std::vector<CellData>	 						m_cellCache;
		std::vector<RE::TESObjectREFR*> 				m_npcRefIds;
		std::unordered_set<const RE::TESFile*> 			m_modList;
		std::set<std::string> 							m_modListSorted;

		std::unordered_set<const RE::TESFile*> 			m_itemModList;
		std::unordered_set<const RE::TESFile*> 			m_npcModList;
		std::unordered_set<const RE::TESFile*> 			m_staticModList;
		std::unordered_set<const RE::TESFile*> 			m_cellModList;

		std::set<std::string> 							m_npcClassList;
		std::set<std::string> 							m_npcRaceList;
		std::set<std::string> 							m_npcFactionList;
		std::unordered_map<const RE::TESFile*, ModFileItemFlags> 	m_itemListModFormTypeMap;
		std::unordered_map<const RE::TESFile*, std::time_t> 		m_modListLastModified;

		void ApplyModFileItemFlags(const RE::TESFile* a_mod, RE::FormType a_formType);

		template <class T>
		void CacheItems(RE::TESDataHandler* a_data);

		template <class T>
		void CacheNPCs(RE::TESDataHandler* a_data);

		template <class T>
		void CacheStaticObjects(RE::TESDataHandler* a_data);

		void CacheCells(RE::TESFile* a_file, std::map<std::tuple<std::uint32_t, const std::string, const std::string>, std::string_view>& out_map);
		void MergeNPCRefIds(std::shared_ptr<std::unordered_map<RE::FormID, RE::FormID>> npc_ref_map);
	};
}
