#pragma once

#include "localization/Locale.h"
#include "pch.h"
#include "external/magic_enum.hpp"
#include "external/icons/IconsLucide.h"
#include "config/UserConfig.h"

#include <format>
#include <optional>

namespace Modex
{
	// https://github.com/Nightfallstorm/DescriptionFramework | License GPL-3.0
	using _GetFormEditorID = const char* (*)(std::uint32_t);
	std::string po3_GetEditorID(RE::FormID a_formID);

	class Kit;

	enum class PropertyType : uint32_t
	{
		kNone = 0,
		kArmor, 				// Handled Form Types
		kBook,
		kKeyMaster,
		kIngredient,
		kAmmo,
		kAlchemy,
		kScroll,
		kMisc,
		kWeapon,
		kNPC,
		kContainer,
		kStatic,
		kLight,
		kDoor,
		kFurniture,
		kActivator,
		kTree,
		kFlora,
		kPlugin, 				// General
		kFormID,
		kName,
		kEditorID,
		kFormType,
		kIsWeapon,
		kWeaponDamage, 			// Item Properties
		kWeaponSpeed,
		kWeaponType,
		kWeaponSkill,
		kWeaponCriticalDamage,
		kWeaponDamagePerSecond,
		kWeaponRange,
		kWeaponStagger,
		kIsArmor,
		kArmorRating,
		kArmorSlot,
		kArmorType,
		kPlayable,
		kEnchanted,
		kGoldValue,
		kCarryWeight,
		kReferenceID,			// NPC Properties
		kUnique,
		kEssential,
		kDisabled,
		kHealth,
		kMagicka,
		kStamina,
		kRace,
		kGender,
		kClass,
		kLevel,
		kDefaultOutfit,
		kSleepOutfit,
		kKeyword,
		kKeywordList,
		kFaction,
		kFactionList,
		kSpell,
		kSpellList,
		kCell,					// Location Properties
		kLand,
		kSpellCost,				// Spell Properties
		kSpellType,
		kSpellCastType,
		kSpellDelivery,
		kTomeSpell,
		kTomeSkill,
		kImGuiSeparator,        // Special ImGui Separator
		kKitItemCount,          // Kit properties
		kTotal
	};

	// Used to represent a property that can be filtered or sorted on
	struct FilterProperty
	{
	private:
		PropertyType		m_type;

	public:
		FilterProperty(PropertyType a_type) :
			m_type{ a_type }
		{}

		bool operator==(const FilterProperty& a_rhs) const { return m_type == a_rhs.m_type; }
		bool operator!=(const FilterProperty& a_rhs) const { return !(*this == a_rhs); }

		const PropertyType& GetPropertyType() const { return m_type; }

		std::string ToString(bool a_prefix = false) const
		{
			auto name = magic_enum::enum_name(m_type);

			if (name.empty()) {
				return "Unknown";
			}

			if (!a_prefix && name.starts_with("k")) {
				name.remove_prefix(1);
			}

			return std::string{name};
		}

		static std::string GetString(PropertyType a_type)
		{
			auto key = magic_enum::enum_name(a_type);

			if (key.empty()) {
				return "Modex Error";
			}

			return Translate(key.data());
		}
		
		static std::string GetPropertyTooltipKey(PropertyType a_property)
		{
			return std::string(magic_enum::enum_name(a_property).data()) + "_TOOLTIP";
		}

		static std::string GetIcon(PropertyType a_type)
		{
			switch (a_type) {
			case PropertyType::kArmor:
				return ICON_LC_SHIELD;
			case PropertyType::kBook:
				return ICON_LC_BOOK_OPEN;
			case PropertyType::kKeyMaster:
				return ICON_LC_KEY;
			case PropertyType::kIngredient:
				return ICON_LC_TEST_TUBE;
			case PropertyType::kAmmo:
				return ICON_LC_CONTAINER;
			case PropertyType::kAlchemy:
				return ICON_LC_FLASK_CONICAL;
			case PropertyType::kScroll:
				return ICON_LC_SCROLL;
			case PropertyType::kMisc:
				return ICON_LC_CUBOID;
			case PropertyType::kWeapon:
				return ICON_LC_SWORD;
			case PropertyType::kNPC:
				return ICON_LC_USER;
			case PropertyType::kContainer:
				return ICON_LC_PACKAGE;
			case PropertyType::kStatic:
				return ICON_LC_SHAPES;
			case PropertyType::kLight:
				return ICON_LC_SUN;
			case PropertyType::kDoor:
				return ICON_LC_DOOR_CLOSED;
			case PropertyType::kFurniture:
				return ICON_LC_ARMCHAIR;
			case PropertyType::kActivator:
				return ICON_LC_TARGET;
			case PropertyType::kTree:
				return ICON_LC_TREE_PINE;
			case PropertyType::kFlora:
				return ICON_LC_FLOWER;
			case PropertyType::kPlugin:
				return ICON_LC_BOX;
			case PropertyType::kFormID:
				return ICON_LC_ASTERISK;
			case PropertyType::kName:
				return ICON_LC_FILE_TEXT;
			case PropertyType::kEditorID:
				return ICON_LC_SIGNATURE;
			case PropertyType::kFormType:
				return ICON_LC_BOOKMARK_CHECK;
			case PropertyType::kIsWeapon:
				return ICON_LC_SWORD;
			case PropertyType::kWeaponDamage:
				return ICON_LC_SWORD;
			case PropertyType::kWeaponSpeed:
				return ICON_LC_GAUGE;
			case PropertyType::kWeaponType:
				return ICON_LC_SWORDS;
			case PropertyType::kWeaponSkill:
				return ICON_LC_BOOK_USER;
			case PropertyType::kWeaponCriticalDamage:
				return ICON_LC_DICES;
			case PropertyType::kWeaponDamagePerSecond:
				return ICON_LC_SWORD;
			case PropertyType::kWeaponRange:
				return ICON_LC_CHEVRONS_LEFT_RIGHT;
			case PropertyType::kWeaponStagger:
				return ICON_LC_SCALE;
			case PropertyType::kIsArmor:
				return ICON_LC_SHIELD;
			case PropertyType::kArmorRating:
				return ICON_LC_SHIELD;
			case PropertyType::kArmorSlot:
				return ICON_LC_BETWEEN_HORIZONTAL_START;
			case PropertyType::kArmorType:
				return ICON_LC_BOOK_USER;
			case PropertyType::kPlayable:
				return ICON_LC_USER_CHECK;
			case PropertyType::kEnchanted:
				return ICON_LC_SPARKLES;
			case PropertyType::kGoldValue:
				return ICON_LC_COINS;
			case PropertyType::kCarryWeight:
				return ICON_LC_WEIGHT;
			case PropertyType::kReferenceID:
				return ICON_LC_KEY_ROUND;
			case PropertyType::kUnique:
				return ICON_LC_STAR;
			case PropertyType::kEssential:
				return ICON_LC_SHIELD_ALERT;
			case PropertyType::kDisabled:
				return ICON_LC_CIRCLE_X;
			case PropertyType::kHealth:
				return ICON_LC_HEART;
			case PropertyType::kMagicka:
				return ICON_LC_SPARKLES;
			case PropertyType::kStamina:
				return ICON_LC_DUMBBELL;
			case PropertyType::kRace:
				return ICON_LC_USER;
			case PropertyType::kGender:
				return ICON_LC_PERSON_STANDING;
			case PropertyType::kClass:
				return ICON_LC_GRADUATION_CAP;
			case PropertyType::kLevel:
				return ICON_LC_ARROW_BIG_UP_DASH;
			case PropertyType::kFactionList:
				return ICON_LC_USERS_ROUND;
			case PropertyType::kSpellList:
				return ICON_LC_GRADUATION_CAP;
			case PropertyType::kKeyword:
				return ICON_LC_TAG;
			case PropertyType::kKeywordList:
				return ICON_LC_TAG;
			case PropertyType::kDefaultOutfit:
				return ICON_LC_SHIRT;
			case PropertyType::kSleepOutfit:
				return ICON_LC_SHIRT;
			case PropertyType::kSpell:
				return ICON_LC_WAND; // TODO: Verify if this is a good fit?
			case PropertyType::kSpellCost:
				return ICON_LC_ZAP;
			case PropertyType::kSpellType:
				return ICON_LC_WAND_SPARKLES;
			case PropertyType::kSpellCastType:
				return ICON_LC_HAND;
			case PropertyType::kSpellDelivery:
				return ICON_LC_WAND;
			case PropertyType::kCell:
				return ICON_LC_MAP_PIN;
			case PropertyType::kLand:
				return ICON_LC_MAP_PIN;
			case PropertyType::kTomeSpell:
				return ICON_LC_WAND;
			case PropertyType::kTomeSkill:
				return ICON_LC_BOOK_USER;
			default:
				// ASSERT_MSG(true, "Unhandled PropertyType in GetIcon()");
				return ICON_LC_MESSAGE_CIRCLE_QUESTION;
			}
		}

		static std::string GetIconWithText(PropertyType a_property)
		{
			return GetIcon(a_property) + " " + GetString(a_property);
		}

		// Construct from string (returns nullopt on failure)
		static std::optional<FilterProperty> FromString(const std::string& a_str)
		{
			auto enumOpt = magic_enum::enum_cast<PropertyType>(a_str);
			
			if (enumOpt.has_value()) {
				return FilterProperty{ enumOpt.value() };
			}
			
			// Did we miss the 'k' prefix?
			enumOpt = magic_enum::enum_cast<PropertyType>("k" + a_str);
			
			if (enumOpt.has_value()) {
				return FilterProperty{ enumOpt.value() };
			}

			if (a_str == "<separator>") {
				return FilterProperty{ PropertyType::kImGuiSeparator };
			}

			ASSERT_MSG(true, "FilterProperty -> Failed to convert string to PropertyType: " + a_str);
			
			return std::nullopt;
		}

	};

	// Wrapper around TESForm pointer to provide safe accessors with default fallbacks.
	// This is best-effort to avoid null pointer dereferences at runtime. \_(ツ)_/

	class TESFormWrapper
	{
	private: 
		RE::TESForm* m_form;
		
	public:
		explicit TESFormWrapper(RE::TESForm* form) : m_form(form) {}
		
		[[nodiscard]] bool IsValid() const { return m_form != nullptr; }
		[[nodiscard]] RE::TESForm* Get() const { return m_form; }

		std::string ValidateFilename(int32_t a_compileIndex) const
		{
			if (!IsValid()) return "Error";

			auto* TESFile = m_form->GetFile(a_compileIndex);

			if (!TESFile) return "Error";	
			if (TESFile->fileName[0] == '\0') return "Error";

			return TESFile->fileName;
		}

		// TEST: Changed to EditorID fallback. Verify stability.

		std::string ValidateName() const
		{
			if (!IsValid()) return "Error";
			if (m_form->GetName() == nullptr) return po3_GetEditorID(m_form->formID);
			if (m_form->GetName()[0] == '\0') return po3_GetEditorID(m_form->formID);

			return m_form->GetName();
		}
		
		// Safe accessors with default fallbacks
		[[nodiscard]] const std::string WGetName() const {
			return m_form ? ValidateName() : "[NULL FORM]";
		}
		
		[[nodiscard]] const std::string WGetEditorID() const {
			return m_form ? po3_GetEditorID(m_form->GetFormID()) : "[NULL FORM]";
		}
		
		// BUG: If users change this at runtime, we need to regenerate all objects!!
		[[nodiscard]] const std::string WGetPluginName() const {
			const int32_t idx = UserConfig::GetCompileIndex();
			return m_form ? ValidateFilename(idx) : "[NULL FORM]";
		}
		
		[[nodiscard]] const std::string WGetFormID() const {
			return m_form ? std::format("{:08X}", m_form->GetFormID()) : "[NULL FORM]";
		}

		[[nodiscard]] RE::FormID WGetBaseFormID() const {
			return m_form ? m_form->GetFormID() : 0;
		}
		
		template<typename T>
		[[nodiscard]] std::optional<T*> As() const {
			return m_form ? std::optional<T*>(m_form->As<T>()) : std::nullopt;
		}
	};

	// Base class for all objects represented in the mod explorer.
	// Handle nullptr safety via accessors in this class.

	class BaseObject
	{
	private:
		const TESFormWrapper 	m_formWrapper;
		const std::string 		m_name;
		const std::string 		m_editorid;
		const std::string 		m_plugin;
		const std::string 		m_formid;
		const RE::FormID 		m_baseid;
	public:
		RE::FormID 			m_refID;
		ImGuiID 			m_tableID = 0;
		int					m_quantity = 1;

		// TODO: Reference old comment as to why I should not move this to Kit class.
		// bool				kitEquipped = false;
		// int				kitAmount = 1;
		//
		bool                m_equipped = false;

		// Constructor from TESForm pointer
		BaseObject(RE::TESForm* form, ImGuiID a_id = 0, RE:: FormID a_refID = 0, int a_quantity = 1, bool a_equipped = false)
			: m_formWrapper{ form }
			, m_name{ m_formWrapper.WGetName() }
			, m_editorid{ m_formWrapper.WGetEditorID() }
			, m_plugin{ m_formWrapper.WGetPluginName() }
			, m_formid{ m_formWrapper. WGetFormID() }
			, m_baseid{ m_formWrapper.WGetBaseFormID() }
			, m_refID{ a_refID }
			, m_tableID{ a_id }
			, m_quantity{ a_quantity }
			, m_equipped{ a_equipped }
		{}

		// Explicit dummy object constructor
		BaseObject(std::string a_name, std::string a_editorid, std::string a_plugin, ImGuiID a_id = 0, int a_quantity = 1, bool a_equipped = false) 
			: m_formWrapper{ nullptr }
			, m_name{ a_name }
			, m_editorid{ a_editorid }
			, m_plugin{ a_plugin }
			, m_formid{ "" }
			, m_baseid{ 0 }
			, m_refID{ 0 }
			, m_tableID{ a_id }
			, m_quantity{ a_quantity }
			, m_equipped{ a_equipped }
		{}

		~BaseObject() = default;

		inline bool 					IsDummy() const { return !m_formWrapper.IsValid(); }
		inline int						GetQuantity() const { return m_quantity; }		
		inline bool 					GetEquipped() const { return m_equipped; }
		inline RE::TESForm* 			GetTESForm() const { return m_formWrapper.Get(); }
		inline RE::FormID 				GetBaseFormID() const { return m_baseid; }
		inline RE::FormID 				GetRefID() const { return m_refID; }
		inline ImGuiID 					GetTableID() const { return m_tableID; }

		inline const std::string& 		GetName() const { return m_name; }
		inline const std::string& 		GetFormID() const { return m_formid; }
		inline const std::string& 		GetEditorID() const { return m_editorid; }
		inline const std::string& 		GetPluginName() const { return m_plugin; }
		inline const std::string_view 	GetNameView() const { return m_name; }
		inline const std::string_view 	GetEditorIDView() const { return m_editorid; }
		inline const std::string_view	GetPluginNameView() const { return m_plugin; }

		// std::optional for nullptr safety.
		inline std::optional<RE::TESFile*> GetFile(int32_t a_idx = 0) const
		{
			if (!m_formWrapper.IsValid()) return std::nullopt;
			RE::TESFile* file = m_formWrapper.Get()->GetFile(a_idx);
			if (file == nullptr) return std::nullopt;
			return file;
		}

		// std::optional for nullptr safety.
		inline const std::optional<RE::TESFileArray*> GetFileArray() const
		{
			if (!m_formWrapper.IsValid()) return std::nullopt;
			RE::TESFileArray* array = m_formWrapper.Get()->sourceFiles.array;
			if (array == nullptr) return std::nullopt;
			return array;
		}

		inline RE::FormType GetFormType() const
		{
			if (!m_formWrapper.IsValid()) return RE::FormType::None;
			RE::FormType type = m_formWrapper.Get()->GetFormType();
			return (type == RE::FormType::SoulGem) ? RE::FormType::Misc : type;
		}

		inline const std::string_view GetTypeName() const
		{
			if (!m_formWrapper.IsValid()) return "None";
			return RE::FormTypeToString(GetFormType());
		}

		inline int32_t GetWeight() const
		{
			if (!m_formWrapper.IsValid()) return 0;
			return static_cast<int32_t>(m_formWrapper.Get()->GetWeight());
		}
	
		inline int32_t GetGoldValue() const
		{
			if (!m_formWrapper.IsValid()) return 0;
			return m_formWrapper.Get()->GetGoldValue();
		}

		inline const std::string GetGoldValueString() const
		{
			if (!m_formWrapper.IsValid()) return "0";
			return std::format("{:d}", GetGoldValue());
		}

		// NOTE: There might be a need for this type of parse on equipment slots. Arch-mage robes is
		// a horrendous example of where it is assigned EquipSlot three times? Otherwise we can
		// usually derive the slot based on the first index.

		// inline const std::string GetEquipSlot() const
		// {
		// 	// TODO: This needs to be merged into JSON file.
		// 	static const std::unordered_map<std::string_view, std::string_view> keyword_to_slot = {
		// 		{"ArmorHelmet", "Helmet"},
		// 		{"ArmorCuirass", "Cuirass"},
		// 		{"ArmorGauntlets", "Gauntlets"},
		// 		{"ArmorBoots", "Boots"},
		// 		{"ArmorShield", "Shield"},
		// 		{"WeapTypeSword", "Sword"},
		// 	};
		//
		// 	for (const auto& keyword : GetKeywordList()) {
		// 		if (auto it = keyword_to_slot.find(keyword); it != keyword_to_slot.end()) {
		// 			return std::string(it->second);
		// 		}
		// 	}
		//
		// 	return "Unknown";
		// }

		// Returns empty vector if not armor, or no slots are found.
		inline const std::vector<std::string> GetArmorSlots() const
		{
			std::vector<std::string> slots;
			slots.push_back("None");

			if (!m_formWrapper.IsValid()) {
				return slots;
			}

			if (auto bipedObject = GetTESForm()->As<RE::BGSBipedObjectForm>(); bipedObject != nullptr) {
				auto slotMask = bipedObject->GetSlotMask();
				
				// GetArmorSlotName(static_cast<RE::BIPED_MODEL::BipedObjectSlot>(1 << i)));
				
				if (slotMask == RE::BIPED_MODEL::BipedObjectSlot::kNone) {
					return slots;
				}

				slots.clear();

				for (int i = 0; i < 32; i++) {
					if (static_cast<int>(slotMask) & (1 << i)) {
						switch(static_cast<RE::BIPED_MODEL::BipedObjectSlot>(1 << i)) 
						{
							case RE::BIPED_MODEL::BipedObjectSlot::kHead:
								slots.push_back("Head"); // 30
								break;
							case RE::BIPED_MODEL::BipedObjectSlot::kHair:
								slots.push_back("Hair"); // 31
								break;
							case RE::BIPED_MODEL::BipedObjectSlot::kBody:
								slots.push_back("Body"); // 32
								break;
							case RE::BIPED_MODEL::BipedObjectSlot::kHands:
								slots.push_back("Hands"); // 33
								break;
							case RE::BIPED_MODEL::BipedObjectSlot::kForearms:
								slots.push_back("Forearms"); // 34
								break;
							case RE::BIPED_MODEL::BipedObjectSlot::kAmulet:
								slots.push_back("Amulet"); // 35
								break;
							case RE::BIPED_MODEL::BipedObjectSlot::kRing:
								slots.push_back("Ring"); // 36
								break;
							case RE::BIPED_MODEL::BipedObjectSlot::kFeet:
								slots.push_back("Feet"); // 37
								break;
							case RE::BIPED_MODEL::BipedObjectSlot::kCalves:
								slots.push_back("Calves"); // 38
								break;
							case RE::BIPED_MODEL::BipedObjectSlot::kShield:
								slots.push_back("Shield"); // 39
								break;
							case RE::BIPED_MODEL::BipedObjectSlot::kTail:
								slots.push_back("Tail"); // 40
								break;
							case RE::BIPED_MODEL::BipedObjectSlot::kLongHair:
								slots.push_back("Long Hair"); // 41
								break;
							case RE::BIPED_MODEL::BipedObjectSlot::kCirclet:
								slots.push_back("Circlet"); // 42
								break;
							case RE::BIPED_MODEL::BipedObjectSlot::kEars:
								slots.push_back("Ears"); // 43
								break;
							case RE::BIPED_MODEL::BipedObjectSlot::kModMouth:
								slots.push_back("Mod Mouth"); // 44
								break;
							case RE::BIPED_MODEL::BipedObjectSlot::kModNeck:
								slots.push_back("Mod Neck"); // 45
								break;
							case RE::BIPED_MODEL::BipedObjectSlot::kModChestPrimary:
								slots.push_back("Mod Chest Primary"); // 46
								break;
							case RE::BIPED_MODEL::BipedObjectSlot::kModBack:
								slots.push_back("Mod Back"); // 47
								break;
							case RE::BIPED_MODEL::BipedObjectSlot::kModMisc1:
								slots.push_back("Mod Misc 1"); // 48
								break;
							case RE::BIPED_MODEL::BipedObjectSlot::kModPelvisPrimary:
								slots.push_back("Mod Pelvis Primary"); // 49
								break;
							case RE::BIPED_MODEL::BipedObjectSlot::kDecapitateHead:
								slots.push_back("Decapitate Head"); // 50
								break;
							case RE::BIPED_MODEL::BipedObjectSlot::kDecapitate:
								slots.push_back("Decapitate"); // 51
								break;
							case RE::BIPED_MODEL::BipedObjectSlot::kModPelvisSecondary:
								slots.push_back("Mod Pelvis Secondary"); // 52
								break;
							case RE::BIPED_MODEL::BipedObjectSlot::kModLegRight:
								slots.push_back("Mod Leg Right"); // 53
								break;
							case RE::BIPED_MODEL::BipedObjectSlot::kModLegLeft:
								slots.push_back("Mod Leg Left"); // 54
								break;
							case RE::BIPED_MODEL::BipedObjectSlot::kModFaceJewelry:
								slots.push_back("Mod Face Jewelry"); // 55
								break;
							case RE::BIPED_MODEL::BipedObjectSlot::kModChestSecondary:
								slots.push_back("Mod Chest Secondary"); // 56
								break;
							case RE::BIPED_MODEL::BipedObjectSlot::kModShoulder:
								slots.push_back("Mod Shoulder"); // 57
								break;
							case RE::BIPED_MODEL::BipedObjectSlot::kModArmLeft:
								slots.push_back("Mod Arm Left"); // 58
								break;
							case RE::BIPED_MODEL::BipedObjectSlot::kModArmRight:
								slots.push_back("Mod Arm Right"); // 59
								break;
							case RE::BIPED_MODEL::BipedObjectSlot::kModMisc2:
								slots.push_back("Mod Misc 2"); // 60
								break;
							case RE::BIPED_MODEL::BipedObjectSlot::kFX01:
								slots.push_back("FX01"); // 61
								break;
							default:
								ASSERT_MSG(true, "BaseObject -> Unhandled armor slot in GetArmorSlots()");
								slots.push_back("None");
						}
					}
				}
			}

			return slots;
		}

		// TODO: Only works on Weapons ?
		inline bool IsPlayable() const
		{
			switch (GetFormType()) 
			{
				case RE::FormType::Weapon:
					if (auto weapon = m_formWrapper.As<RE::TESObjectWEAP>(); weapon.has_value()) {
						if (weapon.value() == nullptr) 
							return true;

						return !weapon.value()->weaponData.flags.any(RE::TESObjectWEAP::Data::Flag::kNonPlayable);
					}
				case RE::FormType::Armor:
					if (auto armor = m_formWrapper.As<RE::TESObjectARMO>(); armor.has_value()) {
						if (armor.value() == nullptr) 
							return true;

						return !(armor.value()->GetFormFlags() & static_cast<uint32_t>(RE::TESObjectARMO::RecordFlags::kNonPlayable));
					}
				default:
					return true;
			};
		}

		inline bool IsEnchanted() const {
			if (const auto& enchantable = m_formWrapper.As<RE::TESEnchantableForm>(); enchantable.has_value()) {
				if (enchantable.value() == nullptr) 
					return false;
					
				if (enchantable.value()->formEnchanting != nullptr) {
					return true;
				}
			}
			
			return false;
		}

		inline bool IsArmor() const {
			if (auto armor = m_formWrapper.As<RE::TESObjectARMO>(); armor.has_value()) {
				if (armor.value() == nullptr) 
					return false;

				return true;
			}
			return false;
		}

		inline bool IsWeapon() const {
			if (GetFormType() != RE::FormType::Weapon) return false;

			if (auto weapon = m_formWrapper.As<RE::TESObjectWEAP>(); weapon.has_value()) {
				if (weapon.value() == nullptr) 
					return false;

				return true;
			}
			return false;
		}

		inline bool IsItem() const {
			auto formType = GetFormType();

			if (formType == RE::FormType::Armor || formType == RE::FormType::Weapon || 
				formType == RE::FormType::Ammo || formType == RE::FormType::Book || 
				formType == RE::FormType::KeyMaster || formType == RE::FormType::Misc ||
				formType == RE::FormType::SoulGem || formType == RE::FormType::Ingredient ||
				formType == RE::FormType::AlchemyItem || formType == RE::FormType::Apparatus ||
				formType == RE::FormType::Scroll) 
				return true;
			
			return false;
		}

		inline uint32_t GetArmorRating() const {
			if (auto armor = m_formWrapper.As<RE::TESObjectARMO>(); armor.has_value()) {
				if (armor.value() == nullptr) 
					return 0;

				return static_cast<uint32_t>(armor.value()->armorRating / 100);
			}
			return 0;
		}

		inline std::string GetArmorType() const {
			if (auto armor = m_formWrapper.As<RE::TESObjectARMO>(); armor.has_value()) {
				if (armor.value() == nullptr) 
					return "";

				switch (armor.value()->GetArmorType()) 
				{
					case RE::TESObjectARMO::ArmorType::kLightArmor:
						return "Light";
					case RE::TESObjectARMO::ArmorType::kHeavyArmor:
						return "Heavy";
					case RE::TESObjectARMO::ArmorType::kClothing:
						return "Clothing";
					default:
						return "";
				}
			}
			
			return "";
		}

		// Simplified these calculations from previous versions.
		inline uint32_t GetWeaponDamage() const {
			if (auto weapon = m_formWrapper.As<RE::TESObjectWEAP>(); weapon.has_value()) {
				if (weapon.value() == nullptr) 
					return 0;

				return weapon.value()->attackDamage;
			}
			
			return 0;
		}

		inline float GetWeaponCritical() const {
			if (auto weapon = m_formWrapper.As<RE::TESObjectWEAP>(); weapon.has_value()) {
				if (weapon.value() == nullptr) 
					return 0.0f;

				return static_cast<float>(weapon.value()->GetCritDamage());
			}

			return 0.0f;
		}

		// Probably needs reworked.
		inline float GetSpellCost(RE::Actor* a_actor) const {
			if (auto spell = m_formWrapper.As<RE::SpellItem>(); spell.has_value()) {
				if (spell.value() == nullptr) 
					return 0.0f;

				return spell.value()->CalculateMagickaCost(a_actor);
			}

			return 0.0f;
		}

		inline const std::string GetDeliveryType() const {
			if (auto spell = m_formWrapper.As<RE::SpellItem>(); spell.has_value()) {
				if (spell.value() == nullptr) 
					return "";

				switch (spell.value()->data.delivery) 
				{
					case RE::MagicSystem::Delivery::kSelf:
						return "Self";
					case RE::MagicSystem::Delivery::kTouch:
						return "Touch";
					case RE::MagicSystem::Delivery::kAimed:
						return "Aimed";
					case RE::MagicSystem::Delivery::kTargetActor:
						return "Target Actor";
					case RE::MagicSystem::Delivery::kTargetLocation:
						return "Target Location";
					default:
						ASSERT_MSG(true, "BaseObject -> Unhandled delivery type in GetDeliveryType()");
						return "";
				}
			}

			return "";
		}

		inline const std::string GetCastType() const {
			if (auto spell = m_formWrapper.As<RE::SpellItem>(); spell.has_value()) {
				if (spell.value() == nullptr) 
					return "";

				switch (spell.value()->data.castingType) 
				{
					case RE::MagicSystem::CastingType::kConcentration:
						return "Concentration";
					case RE::MagicSystem::CastingType::kConstantEffect:
						return "Constant Effect";
					case RE::MagicSystem::CastingType::kFireAndForget:
						return "Fire and Forget";
					case RE::MagicSystem::CastingType::kScroll:
						return "Scroll";
					default:
						ASSERT_MSG(true, "BaseObject -> Unhandled cast type in GetCastType()");
						return "";
				}
			}
			
			return "";
		}

		inline const std::string GetSpellType() const {
			if (auto spell = m_formWrapper.As<RE::SpellItem>(); spell.has_value()) {
				if (spell.value() == nullptr) 
					return "";

				switch (spell.value()->data.spellType) 
				{
					case RE::MagicSystem::SpellType::kAbility:
						return "Ability";
					case RE::MagicSystem::SpellType::kAddiction:
						return "Addiction";
					case RE::MagicSystem::SpellType::kAlchemy:
						return "Alchemy";
					case RE::MagicSystem::SpellType::kDisease:
						return "Disease";
					case RE::MagicSystem::SpellType::kEnchantment:
						return "Enchantment";
					case RE::MagicSystem::SpellType::kIngredient:
						return "Ingredient";
					case RE::MagicSystem::SpellType::kLesserPower:
						return "Lesser Power";
					case RE::MagicSystem::SpellType::kLeveledSpell:
						return "Leveled Spell";
					case RE::MagicSystem::SpellType::kPoison:
						return "Poison";
					case RE::MagicSystem::SpellType::kPower:
						return "Power";
					case RE::MagicSystem::SpellType::kScroll:
						return "Scroll";
					case RE::MagicSystem::SpellType::kSpell:
						return "Spell";
					case RE::MagicSystem::SpellType::kStaffEnchantment:
						return "Staff Enchantment";
					case RE::MagicSystem::SpellType::kVoicePower:
						return "Voice Power";
					// case RE::MagicSystem::SpellType::kPotion: // Alchemy?
					// case RE::MagicSystem::SpellType::kWortCraft: // Ingredient?
					default:
						ASSERT_MSG(true, "BaseObject -> Unhandled spell type in GetSpellType()");
						return "";
				}
			}
			

			return "";
		}

		// Are there more skills?
		inline const std::string GetWeaponSkill() const {
			if (auto weapon = m_formWrapper.As<RE::TESObjectWEAP>(); weapon.has_value()) {
				if (weapon.value() == nullptr) 
					return "";

				switch (weapon.value()->weaponData.skill.get()) 
				{
					case RE::ActorValue::kUnarmedDamage:
						return "Hand-to-Hand";
					case RE::ActorValue::kOneHanded:
						return "One-Handed";
					case RE::ActorValue::kTwoHanded:
						return "Two-Handed";
					case RE::ActorValue::kArchery:
						return "Archery";
					case RE::ActorValue::kBlock:
						return "Block";
					case RE::ActorValue::kNone:
						return "None";
					case RE::ActorValue::kRestoration: // Staves?
						return "Restoration";
					case RE::ActorValue::kDestruction:
						return "Destruction";
					case RE::ActorValue::kAlteration:
						return "Alteration";
					case RE::ActorValue::kConjuration:
						return "Conjuration";
					case RE::ActorValue::kIllusion:
						return "Illusion";
					default:
						ASSERT_MSG(true, "BaseObject -> Unhandled weapon skill in GetWeaponSkill(): {}" + std::to_string(static_cast<int>(weapon.value()->weaponData.skill.get())));
						return "";
				}
			}

			return "";
		}

		inline float GetWeaponSpeed() const {
			if (auto weapon = m_formWrapper.As<RE::TESObjectWEAP>(); weapon.has_value()) {
				if (weapon.value() == nullptr) 
					return 0.0f;

				return weapon.value()->GetSpeed();
			}
			
			return 0.0f;
		}

		inline const std::optional<RE::TESObjectWEAP*> GetTESWeapon() const
		{
			if (auto weapon = m_formWrapper.As<RE::TESObjectWEAP>(); weapon.has_value()) {
				if (weapon.value() == nullptr) 
					return std::nullopt;
					
				return weapon.value();
			}

			return std::nullopt;
		}

		inline const std::optional<RE::TESObjectARMO*> GetTESArmor() const
		{
			if (auto armor = m_formWrapper.As<RE::TESObjectARMO>(); armor.has_value()) {
				if (armor.value() == nullptr)
					return std::nullopt;
					
				return armor.value();
			}

			return std::nullopt;
		}

		inline const std::string GetWeaponType() const {
			if (auto weapon = m_formWrapper.As<RE::TESObjectWEAP>(); weapon.has_value()) {
				if (weapon.value() == nullptr) 
					return "";

				switch (weapon.value()->GetWeaponType()) {
					case RE::WEAPON_TYPE::kHandToHandMelee:
						return "Hand";
					case RE::WEAPON_TYPE::kOneHandSword:
						return "Sword";
					case RE::WEAPON_TYPE::kOneHandDagger:
						return "Dagger";
					case RE::WEAPON_TYPE::kOneHandAxe:
						return "Axe";
					case RE::WEAPON_TYPE::kOneHandMace:
						return "Mace";
					case RE::WEAPON_TYPE::kTwoHandAxe:
						return "Greataxe";
					case RE::WEAPON_TYPE::kTwoHandSword:
						return "Greatsword";
					case RE::WEAPON_TYPE::kBow:
						return "Bow";
					case RE::WEAPON_TYPE::kStaff:
						return "Staff";
					case RE::WEAPON_TYPE::kCrossbow:
						return "Crossbow";
					case RE::WEAPON_TYPE::kTotal:
						return "Total";
				}
			}
			
			return "";
		}

		inline float GetWeaponRange() const {
			if (auto weapon = m_formWrapper.As<RE::TESObjectWEAP>(); weapon.has_value()) {
				if (weapon.value() == nullptr) 
					return 0.0f;

				return weapon.value()->weaponData.reach;
			}
			
			return 0.0f;
		}

		inline float GetWeaponStagger() const {
			if (auto weapon = m_formWrapper.As<RE::TESObjectWEAP>(); weapon.has_value()) {
				if (weapon.value() == nullptr) 
					return 0.0f;

				return weapon.value()->weaponData.staggerValue;
			}
			
			return 0.0f;
		}

		inline std::optional<RE::TESNPC*> GetTESNPC() const
		{
			if (!m_formWrapper.IsValid()) return std::nullopt;

			if (auto npc = m_formWrapper.As<RE::TESNPC>(); npc.has_value()) {
				if (npc.value() == nullptr) 
					return std::nullopt;
					
				return npc.value();
			}

			return std::nullopt;
		}

		inline bool IsNPC() const
		{
			if (auto npc = GetTESNPC(); npc.has_value()) {
				return true;
			}

			return false;
		}

		inline const std::string GetClass() const
		{
			if (auto npc = GetTESNPC(); npc.has_value()) {
				return npc.value()->npcClass->GetName();
			}

			return "";
		}

		inline const std::string GetRace() const
		{
			if (auto npc = GetTESNPC(); npc.has_value()) {
				return npc.value()->race->GetName();
			}

			return "";
		}

		inline const std::string GetGender() const
		{
			if (auto npc = GetTESNPC(); npc.has_value()) {
				if (npc.value() == nullptr)
					return "";

				return npc.value()->IsFemale() ? "Female" : "Male";
			}

			return "";
		}

		inline uint32_t GetLevel() const
		{
			if (auto npc = GetTESNPC(); npc.has_value()) {
				return npc.value()->GetLevel();
			}

			return 0;
		}

		inline std::vector<std::string> GetSpellList() const
		{
			std::vector<std::string> spells;

			if (auto npc = GetTESNPC(); npc.has_value()) {
				const auto spellList = npc.value()->GetSpellList();

				if (spellList != nullptr && spellList->numSpells > 0) {
					for (uint32_t i = 0; i < spellList->numSpells; i++) {
						if (spellList->spells[i] == nullptr)
							continue;

						const auto* spell = spellList->spells[i];

						if (spell != nullptr) {
							if (spell->GetName() != nullptr) {
								spells.push_back(spell->GetName());
							}
						}
					}
				}
			}

			return spells;
		}

		inline std::string GetDefaultOutfit() const
		{
			if (auto npc = GetTESNPC(); npc.has_value()) {
				if (npc.value()->defaultOutfit == nullptr)
					return "";

				return po3_GetEditorID(npc.value()->defaultOutfit->GetFormID());
			}

			return "";
		}

		inline std::string GetSleepOutfit() const
		{
			if (auto npc = GetTESNPC(); npc.has_value()) {
				if (npc.value()->sleepOutfit == nullptr)
					return "";

				return po3_GetEditorID(npc.value()->sleepOutfit->GetFormID());
			}
			
			return "";
		}

		inline std::vector<std::string> GetKeywordList() const
		{
			std::vector<std::string> keywords;

			if (auto object = GetTESForm()->As<RE::BGSKeywordForm>(); object != nullptr) {
				object->ForEachKeyword([&](const RE::BGSKeyword* a_keyword) {
					keywords.push_back(a_keyword->GetFormEditorID());
					return RE::BSContainer::ForEachResult::kContinue;
				});
			}

			return keywords;
		}

		inline bool IsUnique() const
		{
			if (auto npc = GetTESNPC(); npc.has_value()) {
				return npc.value()->IsUnique();
			}

			return false;
		}

		inline bool IsEssential() const
		{
			if (auto npc = GetTESNPC(); npc.has_value()) {
				return npc.value()->IsEssential();
			}

			return false;
		}

		inline bool IsDisabled() const
		{
			if (auto npc = GetTESNPC(); npc.has_value()) {
				auto targetRefr = RE::TESForm::LookupByID<RE::TESObjectREFR>(m_refID);
				if (targetRefr == nullptr)
					return false;

				return targetRefr->IsDisabled();
			}

			return false;
		}

		inline bool HasFaction(const std::string& a_faction) const
		{
			const auto factions = GetMergedString(GetFactionList());
			return factions.find(a_faction) != std::string::npos ? true : false;
		}

		inline bool HasSpell(const std::string& a_spell) const
		{
			const auto spells = GetMergedString(GetSpellList());
			return spells.find(a_spell) != std::string::npos ? true : false;
		}

		inline bool HasKeyword(const std::string& a_keyword) const
		{
			const auto keywords = GetMergedString(GetKeywordList());
			return keywords.find(a_keyword) != std::string::npos ? true : false;
		}

		inline std::string GetMergedString(const std::vector<std::string>& a_list) const
		{
			std::string out;

			for (auto& entry : a_list) {
				out += entry;
			}

			return out;
		}

		// TODO: Check for current vs base?
		inline int GetActorValue(const RE::ActorValue a_value) const
		{
			if (auto npc = GetTESNPC(); npc.has_value()) {
				return static_cast<int>(npc.value()->GetActorValue(a_value));
			}

			return 0;
		}

		inline std::optional<RE::TESNPC::Skills> GetSkills() const
		{
			if (auto npc = GetTESNPC(); npc.has_value()) {
				return npc.value()->playerSkills;
			}

			return std::nullopt;
		}

		inline std::optional<RE::BSTArray<RE::FACTION_RANK>> GetFactions() const
		{
			if (auto npc = GetTESNPC(); npc.has_value()) {
				return npc.value()->factions;
			} else {
				return std::nullopt;
			}
		}

		inline std::vector<std::string> GetFactionList() const
		{
			std::vector<std::string> factions;

			if (auto factionArray = GetFactions(); factionArray.has_value()) {
				for (auto& factionRank : factionArray.value()) {
					if (factionRank.faction != nullptr) {
						factions.push_back(factionRank.faction->GetName());
					}
				}
			}

			return factions;
		}

		inline std::string GetBookSkill() const
		{
			if (auto book = m_formWrapper.As<RE::TESObjectBOOK>(); book.has_value()) {
				if (book.value() == nullptr) 
					return "";

				// RE::ActorValueToString(RE::ActorValue a_value) doesn't work here. Idk why.

				if (book.value()->TeachesSkill()) {
					auto skillValue = static_cast<RE::ActorValue>(book.value()->GetSkill());
					auto skillName = magic_enum::enum_name(skillValue);
					if (!skillName.empty()) {
						skillName = skillName.substr(1); // remove 'k' prefix
						return std::string(skillName);
					}
				}
			}

			return "";
		}

		inline std::string GetBookSpell() const
		{
			if (auto book = m_formWrapper.As<RE::TESObjectBOOK>(); book.has_value()) {
				if (book.value() == nullptr) 
					return "";

				if (book.value()->TeachesSpell()) {
					if (auto spell = book.value()->GetSpell(); spell != nullptr) {
						if (spell->GetName() != nullptr) {
							return spell->GetName();
						}

						// Fallback?
						if (spell->GetFullName() != nullptr) {
							return spell->GetFullName();
						}
					}
				}
			}

			return "";
		}

		// Returns: icon + property type string
		std::string GetPropertyTypeWithIcon(PropertyType a_property) const
		{
			if (a_property == PropertyType::kNone)
				return "";

			std::string icon = FilterProperty::GetIcon(a_property);
			std::string typeStr = FilterProperty::GetString(a_property);
			return icon + " " + typeStr;
		}

		// Returns: icon + property value string
		std::string GetPropertyValueWithIcon(PropertyType a_property) const
		{
			if (a_property == PropertyType::kNone)
				return "";

			std::string icon = FilterProperty::GetIcon(a_property);
			std::string value = GetPropertyByValue(a_property);

			if (value.empty() or value == "None" or value == "0" or value == "0.0")
				return "";

			return icon + " " + value;
		}

		// Maps Skyrim FormType's to Modex PropertyType system.
		PropertyType GetItemPropertyType() const {
			auto formType = GetFormType();
			switch (formType)
			{
				case RE::FormType::Armor:
					return (PropertyType::kArmor);
				case RE::FormType::AlchemyItem:
					return (PropertyType::kAlchemy);
				case RE::FormType::Ammo:
					return (PropertyType::kAmmo);
				case RE::FormType::Book:
					return (PropertyType::kBook);
				case RE::FormType::Ingredient:
					return (PropertyType::kIngredient);
				case RE::FormType::KeyMaster:
					return (PropertyType::kKeyMaster);
				case RE::FormType::Misc:
					return (PropertyType::kMisc);
				case RE::FormType::Scroll:
					return (PropertyType::kScroll);
				case RE::FormType::Weapon:
					return (PropertyType::kWeapon);
				case RE::FormType::NPC:
					return (PropertyType::kNPC);
				case RE::FormType::Class:
					return (PropertyType::kClass);
				case RE::FormType::Race:
					return (PropertyType::kRace);
				case RE::FormType::Faction:
					return (PropertyType::kFactionList);
				case RE::FormType::Spell:
					return (PropertyType::kSpellType);
				case RE::FormType::Tree:
					return (PropertyType::kTree);
				case RE::FormType::Static:
					return (PropertyType::kStatic);
				case RE::FormType::Container:
					return (PropertyType::kContainer);
				case RE::FormType::Activator:
					return (PropertyType::kActivator);
				case RE::FormType::Light:
					return (PropertyType::kLight);
				case RE::FormType::Door:
					return (PropertyType::kDoor);
				case RE::FormType::Furniture:
					return (PropertyType::kFurniture);
				case RE::FormType::Flora:
					return (PropertyType::kFlora);
				// default: ASSERT_MSG(true, "Unhandled GetItemPropertyType() case in BaseObject. '{}' '{}'", magic_enum::enum_name(formType), GetEditorID());
				default:
					return PropertyType::kNone;
			}
		}
		
		std::string GetItemIcon() const
		{
			return FilterProperty::GetIcon(GetItemPropertyType());
		}

		// Helper for retrieving Property value by FilterProperty.
		std::string GetPropertyByFilter(const FilterProperty& property) const
		{
			return GetPropertyByValue(property.GetPropertyType());
		}

		// For JSON rule evaluation. Convert string to desired PropertyType.
		std::string GetPropertyByString(const std::string& a_name) const
		{
			auto propertyType = magic_enum::enum_cast<PropertyType>(a_name);
			if (propertyType.has_value()) {
				return GetPropertyByValue(propertyType.value());
			} else {
				ASSERT_MSG(true, "BaseObject -> GetProperty(const std::string& a_name): Unhandled property name: " + a_name);
			}
			
			return "";
		}

		// Returns the value of the objects specified property as a string.
		std::string GetPropertyByValue(PropertyType a_property, const std::string& a_arg = "") const {
			switch (a_property)
			{
				case PropertyType::kNone:
				case PropertyType::kTotal:
				case PropertyType::kArmor:
				case PropertyType::kBook:
				case PropertyType::kKeyMaster:
				case PropertyType::kIngredient:
				case PropertyType::kAmmo:
				case PropertyType::kAlchemy:
				case PropertyType::kScroll:
				case PropertyType::kMisc:
				case PropertyType::kWeapon:
				case PropertyType::kNPC:
				case PropertyType::kContainer:
				case PropertyType::kStatic:
				case PropertyType::kLight:
				case PropertyType::kDoor:
				case PropertyType::kFurniture:
				case PropertyType::kActivator:
				case PropertyType::kTree:
				case PropertyType::kFlora:
				case PropertyType::kCell:
				case PropertyType::kLand:
				case PropertyType::kImGuiSeparator:
					return "";
				case PropertyType::kFormType:
					return GetTypeName().data();
				case PropertyType::kName:
					return GetName();
				case PropertyType::kEditorID:
					return GetEditorID();
				case PropertyType::kFormID:
					return GetFormID();
				case PropertyType::kPlugin:
					return GetPluginName();
				case PropertyType::kReferenceID:
					return m_refID == 0 ? "" :std::format("{:08x}", m_refID);
				case PropertyType::kCarryWeight:
					return std::to_string(GetWeight());
				case PropertyType::kGoldValue:
					return GetGoldValueString();
				case PropertyType::kPlayable:
					return IsPlayable() ? "true" : "false";
				case PropertyType::kEnchanted:
					return IsEnchanted() ? "true" : "false";
				case PropertyType::kIsArmor:
					return IsArmor() ? "true" : "false";
				case PropertyType::kArmorSlot:
					return GetArmorSlots().front(); // TODO: handle multiples?
				case PropertyType::kArmorType:
					return GetArmorType();
				case PropertyType::kArmorRating:
					return std::to_string(GetArmorRating());
				case PropertyType::kIsWeapon:
					return IsWeapon() ? "true" : "false";
				case PropertyType::kWeaponType:
					return GetWeaponType();
				case PropertyType::kWeaponDamage:
					return std::to_string(GetWeaponDamage());
				case PropertyType::kWeaponSkill:
					return GetWeaponSkill();
				case PropertyType::kWeaponDamagePerSecond:
					return std::format("{:.2f}", GetWeaponDamage() * GetWeaponSpeed());
				case PropertyType::kWeaponSpeed:
					return std::format("{:.2f}", GetWeaponSpeed());
				case PropertyType::kWeaponCriticalDamage:
					return std::format("{:.2f}", GetWeaponCritical());
				case PropertyType::kWeaponRange:
					return std::format("{:.2f}", GetWeaponRange());
				case PropertyType::kWeaponStagger:
					return std::format("{:.2f}", GetWeaponStagger());
				case PropertyType::kClass:
					return GetClass();
				case PropertyType::kRace:
					return GetRace();
				case PropertyType::kGender:
					return GetGender();
				case PropertyType::kLevel:
					return std::to_string(GetLevel());
				case PropertyType::kHealth:
					return std::to_string(GetActorValue(RE::ActorValue::kHealth));
				case PropertyType::kMagicka:
					return std::to_string(GetActorValue(RE::ActorValue::kMagicka));
				case PropertyType::kStamina:
					return std::to_string(GetActorValue(RE::ActorValue::kStamina));
				case PropertyType::kUnique:
					return IsUnique() ? "true" : "false";
				case PropertyType::kEssential:
					return IsEssential() ? "true" : "false";
				case PropertyType::kDisabled:
					return IsDisabled() ? "true" : "false";
				case PropertyType::kFaction:
					return HasFaction(a_arg) ? "true" : "false";
				case PropertyType::kFactionList:
					return GetMergedString(GetFactionList());
				case PropertyType::kSpellList:
					return GetMergedString(GetSpellList());
				case PropertyType::kKeyword:
					return HasKeyword(a_arg) ? "true" : "false";
				case PropertyType::kKeywordList:
					return GetMergedString(GetKeywordList());
				case PropertyType::kDefaultOutfit:
					return GetDefaultOutfit();
				case PropertyType::kSleepOutfit:
					return GetSleepOutfit();
				case PropertyType::kSpell:
					return HasSpell(a_arg) ? "true" : "false";
				case PropertyType::kSpellCost:
					return ""; // TODO: Needs actor context
				case PropertyType::kSpellDelivery:
					return GetDeliveryType();
				case PropertyType::kSpellCastType:
					return GetCastType();
				case PropertyType::kSpellType:
					return GetSpellType();
				case PropertyType::kTomeSpell:
					return GetBookSpell();
				case PropertyType::kTomeSkill:
					return GetBookSkill();
				case PropertyType::kKitItemCount:
					return std::to_string(m_quantity); 
			}

			ASSERT_MSG(true, "BaseObject -> GetProperty(PropertyType a_property): Unhandled property type: " + std::to_string(static_cast<int>(a_property)));

			return "";
		}
	};

	class CellData
	{
	public:
		const std::string 	filename;
		const std::string 	cellName;
		const std::string 	editorid;

		const RE::TESFile* 	mod;

		[[nodiscard]] inline std::string_view 		GetPluginName() const { return filename; }
		[[nodiscard]] inline std::string_view 		GetPluginNameView() const { return filename; }
		[[nodiscard]] inline std::string_view 		GetCellName() const { return cellName; }
		[[nodiscard]] inline std::string_view 		GetEditorIDView() const { return editorid; }
		[[nodiscard]] inline std::string_view 		GetEditorID() const { return editorid; }  // TODO: separate these.

		CellData(std::string filename, std::string cellName, std::string editorid, const RE::TESFile* mod = nullptr) :
			filename(filename), 
			cellName(cellName), 
			editorid(editorid), 
			mod(mod) 
		{}
	};

	struct KitBase
	{
		std::string  	m_plugin;
		std::string 	m_name;
		std::string 	m_editorid;

		ImGuiID 		m_tableID = 0;
	};

	struct KitItem : KitBase
	{
		int					m_amount;
		bool				m_equipped;
		BaseObject*			m_ref;

		// Custom comparator for equality based on editorid
		bool operator==(const KitItem& other) const
		{
			return this->m_editorid == other.m_editorid;
		}

		// Conversion operator:  KitItem -> BaseObject
		operator BaseObject() const {
			const auto form = RE::TESForm::LookupByEditorID(m_editorid);

			if (form == nullptr) {
				return BaseObject(m_name, m_editorid, m_plugin);
			} else {
				return BaseObject(form);
			}
		}

		operator std::unique_ptr<BaseObject>() const {
			const auto form = RE::TESForm::LookupByEditorID(m_editorid);

			if (form == nullptr) {
				return std::make_unique<BaseObject>(m_name, m_editorid, m_plugin);
			} else {
				return std::make_unique<BaseObject>(form);
			}
		}


	};

	struct KitPerk : KitBase
	{
		int					rank;
	};

	struct KitSpell : KitBase // unused
	{
	};

	struct KitShout : KitBase // unused
	{
	};

	struct KitData {
		std::string m_collection;
		std::string m_key;
		std::filesystem::path m_filepath;

		const std::string GetName() const {
			return m_key;
		}

		const std::string GetNameTail() const {
			const std::string tail = m_filepath.stem().string();
			return tail;
		}

		bool empty() const {
			return m_filepath.empty() && m_collection.empty() && m_key.empty();
		}

		operator bool() const {
			return !empty();
		}
	};

	// TODO: Implement getters and accessors as needed.

	class Kit : public KitData
	{
	public:
		std::string					m_desc;
		std::vector<std::shared_ptr<KitItem>> m_items;

		// runtime
		ImGuiID 					m_tableID = 0;
		int							m_weaponCount;
		int							m_armorCount;
		int							m_miscCount;

		Kit() : 
			m_weaponCount(0),
			m_armorCount(0),
			m_miscCount(0)
		{}
	};
}
