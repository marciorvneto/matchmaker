export const TEMPLATES = {
  demo: `{mass_balance} 2.34 = cos(t) - log(x/y)\n{ratio} t/x = 23\n{spec} t = 2`,

  evaporator: `// --- Single Evaporator Body ---

// 1. Temperatures & Pressures
{T_liquid} TL = Tsat(PV) + BPR(xL, Tsat(PV))
{T_vapor} TV = TL

// 2. Mass Balances
{mass_liq} mL = mF * (xF / xL)
{mass_vap} mV = mF - mL

// 3. Condensate & Heat Transfer
{P_cond} PC = PS
{T_cond} TC = Tsat(PS)
{m_cond} mC = mS
{Heat} Q = U * A * (TC - TL)

// 4. Energy Balances (Residuals)
{Energy_Liq} (mF * h_liq(xF, TF)) + Q = (mL * h_liq(xL, TL)) + (mV * h_vap(TV, PV))
{Energy_Vap} mS * h_satV(TS, PS) = Q + (mC * h_satL(TC, PC))

// 5. System Specifications (Knowns)
{spec_mF} mF = 50
{spec_xF} xF = 0.15
{spec_TF} TF = 25
{spec_PS} PS = 200     // kPa
{spec_TS} TS = 127
{spec_U} U = 2.5       // kW/m².K
{spec_PV} PV = 50      // kPa
{spec_xL} xL = 0.55`,

  flash: `// --- Flash Drum Separation ---
{overall_mass} F = V + L
{component_mass} F * z = (V * y) + (L * x)
{equilibrium} y = K * x
{energy} (F * hF) + Q = (V * hV) + (L * hL)

// Specifications
{spec_F} F = 10
{spec_z} z = 0.5
{spec_Q} Q = 0
{spec_K} K = 2.5
{spec_hF} hF = 50
{spec_hV} hV = 250
{spec_hL} hL = 40`,
};
