#include "mtype.h"

#include <algorithm>
#include <cmath>
#include <unordered_map>

#include "behavior_strategy.h"
#include "creature.h"
#include "field_type.h"
#include "item.h"
#include "itype.h"
#include "mondeath.h"
#include "monstergenerator.h"
#include "string_id.h"
#include "translations.h"

static const itype_id itype_bone( "bone" );
static const itype_id itype_bone_tainted( "bone_tainted" );
static const itype_id itype_fish( "fish" );
static const itype_id itype_human_flesh( "human_flesh" );
static const itype_id itype_meat( "meat" );
static const itype_id itype_meat_tainted( "meat_tainted" );
static const itype_id itype_veggy( "veggy" );
static const itype_id itype_veggy_tainted( "veggy_tainted" );

static const species_id MOLLUSK( "MOLLUSK" );

mtype::mtype()
{
    id = mtype_id::NULL_ID();
    name = pl_translation( "human", "humans" );
    sym = " ";
    color = c_white;
    size = creature_size::medium;
    volume = 62499_ml;
    weight = 81499_gram;
    mat = { material_id( "flesh" ) };
    phase = SOLID;
    def_chance = 0;
    upgrades = false;
    half_life = -1;
    age_grow = -1;
    upgrade_into = mtype_id::NULL_ID();
    upgrade_group = mongroup_id::NULL_ID();

    reproduces = false;
    baby_count = -1;
    baby_monster = mtype_id::NULL_ID();
    baby_egg = itype_id::NULL_ID();

    burn_into = mtype_id::NULL_ID();
    zombify_into = mtype_id::NULL_ID();
    dies.push_back( &mdeath::normal );
    sp_defense = nullptr;
    harvest = harvest_id( "human" );
    luminance = 0;
    bash_skill = 0;

    aggro_character = true;

    flags
    .set( MF_HUMAN )
    .set( MF_BONES )
    .set( MF_LEATHER );
}

std::string mtype::nname( unsigned int quantity ) const
{
    return name.translated( quantity );
}

bool mtype::has_special_attack( const std::string &attack_name ) const
{
    return special_attacks.find( attack_name ) != special_attacks.end();
}

bool mtype::has_flag( m_flag flag ) const
{
    MonsterGenerator::generator().m_flag_usage_stats[flag]++;
    return flags[flag];
}

void mtype::set_flag( m_flag flag, bool state )
{
    flags.set( flag, state );
}

bool mtype::made_of( const material_id &material ) const
{
    return std::ranges::find( mat,  material ) != mat.end();
}

bool mtype::made_of_any( const std::set<material_id> &materials ) const
{
    if( mat.empty() ) {
        return false;
    }

    return std::ranges::any_of( mat, [&materials]( const material_id & e ) {
        return materials.count( e );
    } );
}

bool mtype::has_anger_trigger( mon_trigger trigger ) const
{
    return anger[trigger];
}

bool mtype::has_fear_trigger( mon_trigger trigger ) const
{
    return fear[trigger];
}

bool mtype::has_placate_trigger( mon_trigger trigger ) const
{
    return placate[trigger];
}

bool mtype::in_category( const std::string &category ) const
{
    return categories.find( category ) != categories.end();
}

bool mtype::in_species( const species_id &spec ) const
{
    if( spec == species_id( "ALL" ) ) {
        return true;
    }
    return species.contains( spec );
}

bool mtype::in_species( const species_type &spec ) const
{
    return species_ptrs.contains( &spec );
}
std::vector<std::string> mtype::species_descriptions() const
{
    std::vector<std::string> ret;
    for( const species_id &s : species ) {
        if( !s->description.empty() ) {
            ret.emplace_back( s->description.translated() );
        }
    }
    return ret;
}

bool mtype::same_species( const mtype &other ) const
{
    for( auto &s : species_ptrs ) {
        if( other.in_species( *s ) ) {
            return true;
        }
    }
    return false;
}

field_type_id mtype::bloodType() const
{
    if( has_flag( MF_ACID_BLOOD ) )
        //A monster that has the death effect "ACID" does not need to have acid blood.
    {
        return fd_acid;
    }
    if( has_flag( MF_BILE_BLOOD ) ) {
        return fd_bile;
    }
    if( has_flag( MF_LARVA ) || has_flag( MF_ARTHROPOD_BLOOD ) ) {
        return fd_blood_invertebrate;
    }
    if( made_of( material_id( "veggy" ) ) ) {
        return fd_blood_veggy;
    }
    if( made_of( material_id( "iflesh" ) ) ) {
        return fd_blood_insect;
    }
    if( has_flag( MF_WARM ) && made_of( material_id( "flesh" ) ) ) {
        return fd_blood;
    }
    return fd_null;
}

field_type_id mtype::gibType() const
{
    if( has_flag( MF_LARVA ) || in_species( MOLLUSK ) ) {
        return fd_gibs_invertebrate;
    }
    if( made_of( material_id( "veggy" ) ) ) {
        return fd_gibs_veggy;
    }
    if( made_of( material_id( "iflesh" ) ) ) {
        return fd_gibs_insect;
    }
    if( made_of( material_id( "flesh" ) ) ) {
        return fd_gibs_flesh;
    }
    // There are other materials not listed here like steel, protoplasmic, powder, null, stone, bone
    return fd_null;
}

itype_id mtype::get_meat_itype() const
{
    if( has_flag( MF_POISON ) ) {
        if( made_of( material_id( "flesh" ) ) || made_of( material_id( "hflesh" ) ) ) {
            return itype_meat_tainted;
        } else if( made_of( material_id( "iflesh" ) ) ) {
            //In the future, insects could drop insect flesh rather than plain ol' meat.
            return itype_meat_tainted;
        } else if( made_of( material_id( "veggy" ) ) ) {
            return itype_veggy_tainted;
        } else if( made_of( material_id( "bone" ) ) ) {
            return itype_bone_tainted;
        }
    } else {
        if( made_of( material_id( "flesh" ) ) || made_of( material_id( "hflesh" ) ) ) {
            if( has_flag( MF_HUMAN ) ) {
                return itype_human_flesh;
            } else if( has_flag( MF_AQUATIC ) ) {
                return itype_fish;
            } else {
                return itype_meat;
            }
        } else if( made_of( material_id( "iflesh" ) ) ) {
            //In the future, insects could drop insect flesh rather than plain ol' meat.
            return itype_meat;
        } else if( made_of( material_id( "veggy" ) ) ) {
            return itype_veggy;
        } else if( made_of( material_id( "bone" ) ) ) {
            return itype_bone;
        }
    }
    return itype_id::NULL_ID();
}

int mtype::get_meat_chunks_count() const
{
    const float ch = to_gram( weight ) * ( 0.40f - 0.02f * std::log10( to_gram( weight ) ) );
    return static_cast<int>( ch / to_gram( get_meat_itype()->weight ) );
}

std::string mtype::get_description() const
{
    return description.translated();
}

std::string mtype::get_footsteps() const
{
    for( const species_id &s : species ) {
        return s.obj().get_footsteps();
    }
    return _( "footsteps." );
}

void mtype::set_strategy()
{
    goals.set_strategy( behavior::strategy_map[ "sequential_until_done" ] );
}

void mtype::add_goal( const std::string &goal_id )
{
    goals.add_child( &string_id<behavior::node_t>( goal_id ).obj() );
}

const behavior::node_t *mtype::get_goals() const
{
    return &goals;
}
