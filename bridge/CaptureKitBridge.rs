//! Original C ABI adapter for externally supplied Capture Kit capture rules.
//! Upstream code stays outside this repository and retains its MIT license.

mod data {
    pub struct Rules {
        pub capture_hp_weight: f64,
        pub capture_level_window: f64,
        pub capture_back_bonus: f64,
    }
}
include!(env!("CAPTURE_KIT_BINDING"));

#[repr(C)]
pub struct CkbRules {
    pub hp_weight: f64,
    pub level_window: f64,
    pub unaware_multiplier: f64,
}
#[repr(C)]
pub struct CkbInputs {
    pub orb_level: f64,
    pub creature_level: u32,
    pub reserved: u32,
    pub hp: f64,
    pub max_hp: f64,
    pub catch_rate: f64,
    pub status_bonus: f64,
    pub unaware: u32,
    pub uncapturable: u32,
}
#[repr(C)]
pub struct CkbBreakdown {
    pub hp_factor: f64,
    pub level_factor: f64,
    pub chance: f64,
}

#[no_mangle]
pub extern "C" fn ckb_api_version() -> u32 { 1 }

/// Callers supply valid, aligned, nonoverlapping buffers of the header's sizes.
#[no_mangle]
pub unsafe extern "C" fn ckb_v1_capture_chance(
    rules: *const CkbRules, input: *const CkbInputs, output: *mut CkbBreakdown,
) -> i32 {
    if rules.is_null() || input.is_null() || output.is_null() { return -1; }
    let (r, i) = unsafe { (&*rules, &*input) };
    if ![r.hp_weight, r.level_window, r.unaware_multiplier, i.orb_level,
          i.hp, i.max_hp, i.catch_rate, i.status_bonus].iter().all(|v| v.is_finite())
        || !(0.0..=1.0).contains(&r.hp_weight) || r.level_window <= 0.0
        || r.unaware_multiplier < 0.0 || i.orb_level < 0.0 || i.creature_level == 0
        || i.hp <= 0.0 || i.max_hp < i.hp || i.catch_rate < 0.0 || i.status_bonus < 0.0
        || i.unaware > 1 || i.uncapturable > 1 || i.reserved != 0 { return -2; }
    // The wrapper rejects invalid/dead inputs before invoking upstream math.
    let rules = data::Rules { capture_hp_weight: r.hp_weight,
        capture_level_window: r.level_window, capture_back_bonus: r.unaware_multiplier };
    let inputs = upstream_capture::CaptureInputs { orb_level: i.orb_level,
        creature_level: i.creature_level, hp: i.hp, max_hp: i.max_hp,
        catch_rate: i.catch_rate, status_bonus: i.status_bonus,
        unaware: i.unaware != 0, uncapturable: i.uncapturable != 0 };
    let result = upstream_capture::capture_chance(&rules, &inputs);
    if ![result.hp_factor, result.level_factor, result.result].iter().all(|v| v.is_finite()) { return -2; }
    unsafe { output.write(CkbBreakdown { hp_factor: result.hp_factor,
        level_factor: result.level_factor, chance: result.result }); }
    0
}

#[no_mangle]
pub unsafe extern "C" fn ckb_v1_judge_shakes(
    chance: f64, weights: *const f64, rolls: *const f64, count: u32, passed: *mut u32,
) -> i32 {
    if weights.is_null() || rolls.is_null() || passed.is_null() { return -1; }
    if !(1..=8).contains(&count) || !chance.is_finite() || !(0.0..=1.0).contains(&chance) { return -2; }
    let weights = unsafe { std::slice::from_raw_parts(weights, count as usize) };
    let rolls = unsafe { std::slice::from_raw_parts(rolls, count as usize) };
    if !weights.iter().all(|w| w.is_finite() && *w > 0.0)
        || !rolls.iter().all(|r| r.is_finite() && (0.0..1.0).contains(r))
        || (weights.iter().sum::<f64>() - 1.0).abs() > 1e-8 { return -2; }
    unsafe { passed.write(upstream_capture::judge_shakes(chance, weights, rolls) as u32); }
    0
}
