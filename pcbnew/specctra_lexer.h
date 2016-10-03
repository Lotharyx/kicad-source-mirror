
/* Do not modify this file it was automatically generated by the
 * TokenList2DsnLexer CMake script.
 */

#ifndef SPECCTRA_LEXER_H_
#define SPECCTRA_LEXER_H_

#include <dsnlexer.h>

/**
 * C++ does not put enum _values_ in separate namespaces unless the enum itself
 * is in a separate namespace.  All the token enums must be in separate namespaces
 * otherwise the C++ compiler will eventually complain if it sees more than one
 * DSNLEXER in the same compilation unit, say by mutliple header file inclusion.
 * Plus this also enables re-use of the same enum name T.  A typedef can always be used
 * to clarify which enum T is in play should that ever be a problem.  This is
 * unlikely since Parse() functions will usually only be exposed to one header
 * file like this one.  But if there is a problem, then use:
 *   typedef DSN::T T;
 * within that problem area.
 */
namespace DSN
{
    /// enum T contains all this lexer's tokens.
    enum T
    {
        // these first few are negative special ones for syntax, and are
        // inherited from DSNLEXER.
        T_NONE          = DSN_NONE,
        T_COMMENT       = DSN_COMMENT,
        T_STRING_QUOTE  = DSN_STRING_QUOTE,
        T_QUOTE_DEF     = DSN_QUOTE_DEF,
        T_DASH          = DSN_DASH,
        T_SYMBOL        = DSN_SYMBOL,
        T_NUMBER        = DSN_NUMBER,
        T_RIGHT         = DSN_RIGHT,        // right bracket: ')'
        T_LEFT          = DSN_LEFT,         // left bracket:  '('
        T_STRING        = DSN_STRING,       // a quoted string, stripped of the quotes
        T_EOF           = DSN_EOF,          // special case for end of file

        T_absolute = 0,
        T_add_group,
        T_add_pins,
        T_added,
        T_allow_antenna,
        T_allow_redundant_wiring,
        T_amp,
        T_ancestor,
        T_antipad,
        T_aperture_type,
        T_array,
        T_attach,
        T_attr,
        T_average_pair_length,
        T_back,
        T_base_design,
        T_bbv_ctr2ctr,
        T_bend_keepout,
        T_bond,
        T_both,
        T_bottom,
        T_bottom_layer_sel,
        T_boundary,
        T_brickpat,
        T_bundle,
        T_bus,
        T_bypass,
        T_capacitance_resolution,
        T_capacitor,
        T_case_sensitive,
        T_cct1,
        T_cct1a,
        T_center_center,
        T_checking_trim_by_pin,
        T_circ,
        T_circle,
        T_circuit,
        T_class,
        T_class_class,
        T_classes,
        T_clear,
        T_clearance,
        T_cluster,
        T_cm,
        T_color,
        T_colors,
        T_comment,
        T_comp,
        T_comp_edge_center,
        T_comp_order,
        T_component,
        T_composite,
        T_conductance_resolution,
        T_conductor,
        T_conflict,
        T_connect,
        T_constant,
        T_contact,
        T_control,
        T_corner,
        T_corners,
        T_cost,
        T_created_time,
        T_cross,
        T_crosstalk_model,
        T_current_resolution,
        T_delete_pins,
        T_deleted,
        T_deleted_keepout,
        T_delta,
        T_diagonal,
        T_direction,
        T_directory,
        T_discrete,
        T_effective_via_length,
        T_elongate_keepout,
        T_exclude,
        T_expose,
        T_extra_image_directory,
        T_family,
        T_family_family,
        T_family_family_spacing,
        T_fanout,
        T_farad,
        T_file,
        T_fit,
        T_fix,
        T_flip_style,
        T_floor_plan,
        T_footprint,
        T_forbidden,
        T_force_to_terminal_point,
        T_forgotten,
        T_free,
        T_fromto,
        T_front,
        T_front_only,
        T_gap,
        T_gate,
        T_gates,
        T_generated_by_freeroute,
        T_global,
        T_grid,
        T_group,
        T_group_set,
        T_guide,
        T_hard,
        T_height,
        T_high,
        T_history,
        T_horizontal,
        T_host_cad,
        T_host_version,
        T_image,
        T_image_conductor,
        T_image_image,
        T_image_image_spacing,
        T_image_outline_clearance,
        T_image_set,
        T_image_type,
        T_inch,
        T_include,
        T_include_pins_in_crosstalk,
        T_inductance_resolution,
        T_insert,
        T_instcnfg,
        T_inter_layer_clearance,
        T_jumper,
        T_junction_type,
        T_keepout,
        T_kg,
        T_kohm,
        T_large,
        T_large_large,
        T_layer,
        T_layer_depth,
        T_layer_noise_weight,
        T_layer_pair,
        T_layer_rule,
        T_length,
        T_length_amplitude,
        T_length_factor,
        T_length_gap,
        T_library,
        T_library_out,
        T_limit,
        T_limit_bends,
        T_limit_crossing,
        T_limit_vias,
        T_limit_way,
        T_linear,
        T_linear_interpolation,
        T_load,
        T_lock_type,
        T_logical_part,
        T_logical_part_mapping,
        T_low,
        T_match_fromto_delay,
        T_match_fromto_length,
        T_match_group_delay,
        T_match_group_length,
        T_match_net_delay,
        T_match_net_length,
        T_max_delay,
        T_max_len,
        T_max_length,
        T_max_noise,
        T_max_restricted_layer_length,
        T_max_stagger,
        T_max_stub,
        T_max_total_delay,
        T_max_total_length,
        T_max_total_vias,
        T_medium,
        T_mhenry,
        T_mho,
        T_microvia,
        T_mid_driven,
        T_mil,
        T_min_gap,
        T_mirror,
        T_mirror_first,
        T_mixed,
        T_mm,
        T_negative_diagonal,
        T_net,
        T_net_number,
        T_net_out,
        T_net_pin_changes,
        T_nets,
        T_network,
        T_network_out,
        T_no,
        T_noexpose,
        T_noise_accumulation,
        T_noise_calculation,
        T_normal,
        T_object_type,
        T_off,
        T_off_grid,
        T_offset,
        T_on,
        T_open,
        T_opposite_side,
        T_order,
        T_orthogonal,
        T_outline,
        T_overlap,
        T_pad,
        T_pad_pad,
        T_padstack,
        T_pair,
        T_parallel,
        T_parallel_noise,
        T_parallel_segment,
        T_parser,
        T_part_library,
        T_path,
        T_pcb,
        T_permit_orient,
        T_permit_side,
        T_physical,
        T_physical_part_mapping,
        T_piggyback,
        T_pin,
        T_pin_allow,
        T_pin_cap_via,
        T_pin_via_cap,
        T_pin_width_taper,
        T_pins,
        T_pintype,
        T_place,
        T_place_boundary,
        T_place_control,
        T_place_keepout,
        T_place_rule,
        T_placement,
        T_plan,
        T_plane,
        T_pn,
        T_point,
        T_polygon,
        T_polyline_path,
        T_position,
        T_positive_diagonal,
        T_power,
        T_power_dissipation,
        T_power_fanout,
        T_prefix,
        T_primary,
        T_priority,
        T_property,
        T_protect,
        T_qarc,
        T_quarter,
        T_radius,
        T_ratio,
        T_ratio_tolerance,
        T_rect,
        T_reduced,
        T_region,
        T_region_class,
        T_region_class_class,
        T_region_net,
        T_relative_delay,
        T_relative_group_delay,
        T_relative_group_length,
        T_relative_length,
        T_reorder,
        T_reroute_order_viols,
        T_resistance_resolution,
        T_resistor,
        T_resolution,
        T_restricted_layer_length_factor,
        T_room,
        T_rotate,
        T_rotate_first,
        T_round,
        T_roundoff_rotation,
        T_route,
        T_route_to_fanout_only,
        T_routes,
        T_routes_include,
        T_rule,
        T_same_net_checking,
        T_sample_window,
        T_saturation_length,
        T_sec,
        T_secondary,
        T_self,
        T_sequence_number,
        T_session,
        T_set_color,
        T_set_pattern,
        T_shape,
        T_shield,
        T_shield_gap,
        T_shield_loop,
        T_shield_tie_down_interval,
        T_shield_width,
        T_side,
        T_signal,
        T_site,
        T_small,
        T_smd,
        T_snap,
        T_snap_angle,
        T_soft,
        T_source,
        T_space_in_quoted_tokens,
        T_spacing,
        T_spare,
        T_spiral_via,
        T_square,
        T_stack_via,
        T_stack_via_depth,
        T_standard,
        T_starburst,
        T_status,
        T_structure,
        T_structure_out,
        T_subgate,
        T_subgates,
        T_substituted,
        T_such,
        T_suffix,
        T_super_placement,
        T_supply,
        T_supply_pin,
        T_swapping,
        T_switch_window,
        T_system,
        T_tandem_noise,
        T_tandem_segment,
        T_tandem_shield_overhang,
        T_term_only,
        T_terminal,
        T_terminator,
        T_test,
        T_test_points,
        T_testpoint,
        T_threshold,
        T_time_length_factor,
        T_time_resolution,
        T_tjunction,
        T_tolerance,
        T_top,
        T_topology,
        T_total,
        T_track_id,
        T_turret,
        T_type,
        T_um,
        T_unassigned,
        T_unconnects,
        T_unit,
        T_up,
        T_use_array,
        T_use_layer,
        T_use_net,
        T_use_via,
        T_value,
        T_vertical,
        T_via,
        T_via_array_template,
        T_via_at_smd,
        T_via_keepout,
        T_via_number,
        T_via_rotate_first,
        T_via_site,
        T_via_size,
        T_virtual_pin,
        T_volt,
        T_voltage_resolution,
        T_was_is,
        T_way,
        T_weight,
        T_width,
        T_window,
        T_wire,
        T_wire_keepout,
        T_wires,
        T_wires_include,
        T_wiring,
        T_write_resolution,
        T_x,
        T_xy,
        T_y
    };
}   // namespace DSN


/**
 * Class SPECCTRA_LEXER
 * is an automatically generated class using the TokenList2DnsLexer.cmake
 * technology, based on keywords provided by file:
 *    /vault/home/brian/git/kicad-source-mirror/pcbnew/specctra.keywords
 */
class SPECCTRA_LEXER : public DSNLEXER
{
    /// Auto generated lexer keywords table and length:
    static const KEYWORD  keywords[];
    static const unsigned keyword_count;

public:
    /**
     * Constructor ( const std::string&, const wxString& )
     * @param aSExpression is (utf8) text possibly from the clipboard that you want to parse.
     * @param aSource is a description of the origin of @a aSExpression, such as a filename.
     *   If left empty, then _("clipboard") is used.
     */
    SPECCTRA_LEXER( const std::string& aSExpression, const wxString& aSource = wxEmptyString ) :
        DSNLEXER( keywords, keyword_count, aSExpression, aSource )
    {
    }

    /**
     * Constructor ( FILE* )
     * takes @a aFile already opened for reading and @a aFilename as parameters.
     * The opened file is assumed to be positioned at the beginning of the file
     * for purposes of accurate line number reporting in error messages.  The
     * FILE is closed by this instance when its destructor is called.
     * @param aFile is a FILE already opened for reading.
     * @param aFilename is the name of the opened file, needed for error reporting.
     */
    SPECCTRA_LEXER( FILE* aFile, const wxString& aFilename ) :
        DSNLEXER( keywords, keyword_count, aFile, aFilename )
    {
    }

    /**
     * Constructor ( LINE_READER* )
     * intializes a lexer and prepares to read from @a aLineReader which
     * is assumed ready, and may be in use by other DSNLEXERs also.  No ownership
     * is taken of @a aLineReader. This enables it to be used by other lexers also.
     * The transition between grammars in such a case, must happen on a text
     * line boundary, not within the same line of text.
     *
     * @param aLineReader is any subclassed instance of LINE_READER, such as
     *  STRING_LINE_READER or FILE_LINE_READER.  No ownership is taken of aLineReader.
     */
    SPECCTRA_LEXER( LINE_READER* aLineReader ) :
        DSNLEXER( keywords, keyword_count, aLineReader )
    {
    }

    /**
     * Function TokenName
     * returns the name of the token in ASCII form.
     */
    static const char* TokenName( DSN::T aTok );

    /**
     * Function NextTok
     * returns the next token found in the input file or T_EOF when reaching
     * the end of file.  Users should wrap this function to return an enum
     * to aid in grammar debugging while running under a debugger, but leave
     * this lower level function returning an int (so the enum does not collide
     * with another usage).
     * @return DSN::T - the type of token found next.
     * @throw IO_ERROR - only if the LINE_READER throws it.
     */
    DSN::T NextTok() throw( IO_ERROR )
    {
        return (DSN::T) DSNLEXER::NextTok();
    }

    /**
     * Function NeedSYMBOL
     * calls NextTok() and then verifies that the token read in
     * satisfies bool IsSymbol().
     * If not, an IO_ERROR is thrown.
     * @return int - the actual token read in.
     * @throw IO_ERROR, if the next token does not satisfy IsSymbol()
     */
    DSN::T NeedSYMBOL() throw( IO_ERROR )
    {
        return (DSN::T) DSNLEXER::NeedSYMBOL();
    }

    /**
     * Function NeedSYMBOLorNUMBER
     * calls NextTok() and then verifies that the token read in
     * satisfies bool IsSymbol() or tok==T_NUMBER.
     * If not, an IO_ERROR is thrown.
     * @return int - the actual token read in.
     * @throw IO_ERROR, if the next token does not satisfy the above test
     */
    DSN::T NeedSYMBOLorNUMBER() throw( IO_ERROR )
    {
        return (DSN::T) DSNLEXER::NeedSYMBOLorNUMBER();
    }

    /**
     * Function CurTok
     * returns whatever NextTok() returned the last time it was called.
     */
    DSN::T CurTok()
    {
        return (DSN::T) DSNLEXER::CurTok();
    }

    /**
     * Function PrevTok
     * returns whatever NextTok() returned the 2nd to last time it was called.
     */
    DSN::T PrevTok()
    {
        return (DSN::T) DSNLEXER::PrevTok();
    }
};

// example usage

/**
 * Class _PARSER
 * holds data and functions pertinent to parsing a S-expression file .
 *
class SPECCTRA_PARSER : public SPECCTRA_LEXER
{

};
*/

#endif   // SPECCTRA_LEXER_H_
