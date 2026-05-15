import math

sequence = [0, 32, 15, 19, 4, 21, 2, 25, 17, 34, 6, 27, 13, 36, 11, 30, 8, 23, 10, 5, 24, 16, 33, 1, 20, 14, 31, 9, 22, 18, 29, 7, 28, 12, 35, 3, 26]

# Standard European Roulette colors
reds = {1,3,5,7,9,12,14,16,18,19,21,23,25,27,30,32,34,36}

def get_color(num):
    if num == 0: return "#008a27" # Green
    if num in reds: return "#d11111" # Red
    return "#111111" # Black

size = 800
cx = size / 2
cy = size / 2

svg = []
svg.append(f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {size} {size}" width="{size}" height="{size}">')

# Definitions for gradients
svg.append('''
  <defs>
    <radialGradient id="wood" cx="50%" cy="50%" r="50%">
      <stop offset="70%" stop-color="#4a2511" />
      <stop offset="100%" stop-color="#2a1205" />
    </radialGradient>
    <radialGradient id="gold" cx="50%" cy="50%" r="50%">
      <stop offset="0%" stop-color="#fff1a0" />
      <stop offset="50%" stop-color="#d4af37" />
      <stop offset="100%" stop-color="#aa7c11" />
    </radialGradient>
    <radialGradient id="inner-gold" cx="50%" cy="50%" r="50%">
      <stop offset="0%" stop-color="#f8e58c" />
      <stop offset="80%" stop-color="#c19b2c" />
      <stop offset="100%" stop-color="#6b5311" />
    </radialGradient>
    <radialGradient id="center-bg" cx="50%" cy="50%" r="50%">
      <stop offset="0%" stop-color="#3a1b0a" />
      <stop offset="100%" stop-color="#1a0a02" />
    </radialGradient>
  </defs>
''')

# Outer wood frame
svg.append(f'<circle cx="{cx}" cy="{cy}" r="390" fill="url(#wood)" stroke="#111" stroke-width="10"/>')

# Outer gold ring
svg.append(f'<circle cx="{cx}" cy="{cy}" r="340" fill="none" stroke="url(#gold)" stroke-width="8"/>')

angle_step = 360 / 37

# Number pockets ring
inner_r = 210
outer_r = 330
text_r = 270

for i, num in enumerate(sequence):
    start_angle = i * angle_step - 90 - (angle_step / 2)
    end_angle = (i + 1) * angle_step - 90 - (angle_step / 2)
    
    # SVG path for a pie slice
    start_rad = math.radians(start_angle)
    end_rad = math.radians(end_angle)
    
    x1 = cx + outer_r * math.cos(start_rad)
    y1 = cy + outer_r * math.sin(start_rad)
    x2 = cx + outer_r * math.cos(end_rad)
    y2 = cy + outer_r * math.sin(end_rad)
    
    x3 = cx + inner_r * math.cos(end_rad)
    y3 = cy + inner_r * math.sin(end_rad)
    x4 = cx + inner_r * math.cos(start_rad)
    y4 = cy + inner_r * math.sin(start_rad)
    
    color = get_color(num)
    
    path = f'M {x1} {y1} A {outer_r} {outer_r} 0 0 1 {x2} {y2} L {x3} {y3} A {inner_r} {inner_r} 0 0 0 {x4} {y4} Z'
    svg.append(f'<path d="{path}" fill="{color}" stroke="#d4af37" stroke-width="2"/>')
    
    # Text
    mid_angle = (start_angle + end_angle) / 2
    mid_rad = math.radians(mid_angle)
    tx = cx + text_r * math.cos(mid_rad)
    ty = cy + text_r * math.sin(mid_rad)
    
    # Rotate text to face center
    rotation = mid_angle + 90
    svg.append(f'<text x="{tx}" y="{ty}" font-family="Arial, sans-serif" font-weight="bold" font-size="28" fill="#fff" text-anchor="middle" dominant-baseline="middle" transform="rotate({rotation}, {tx}, {ty})">{num}</text>')

    # Pocket slots (where ball lands)
    slot_r_outer = 210
    slot_r_inner = 170
    sx1 = cx + slot_r_outer * math.cos(start_rad)
    sy1 = cy + slot_r_outer * math.sin(start_rad)
    sx2 = cx + slot_r_outer * math.cos(end_rad)
    sy2 = cy + slot_r_outer * math.sin(end_rad)
    sx3 = cx + slot_r_inner * math.cos(end_rad)
    sy3 = cy + slot_r_inner * math.sin(end_rad)
    sx4 = cx + slot_r_inner * math.cos(start_rad)
    sy4 = cy + slot_r_inner * math.sin(start_rad)
    
    slot_path = f'M {sx1} {sy1} A {slot_r_outer} {slot_r_outer} 0 0 1 {sx2} {sy2} L {sx3} {sy3} A {slot_r_inner} {slot_r_inner} 0 0 0 {sx4} {sy4} Z'
    svg.append(f'<path d="{slot_path}" fill="{color}" stroke="#222" stroke-width="1"/>')
    # Inner gold separator for slots
    svg.append(f'<line x1="{sx4}" y1="{sy4}" x2="{sx1}" y2="{sy1}" stroke="url(#gold)" stroke-width="3"/>')

# Inner center background
svg.append(f'<circle cx="{cx}" cy="{cy}" r="170" fill="url(#center-bg)" stroke="url(#gold)" stroke-width="6"/>')

# Center golden turret / spindle
svg.append(f'<circle cx="{cx}" cy="{cy}" r="60" fill="url(#gold)" />')
svg.append(f'<circle cx="{cx}" cy="{cy}" r="40" fill="url(#inner-gold)" />')
svg.append(f'<circle cx="{cx}" cy="{cy}" r="20" fill="#f8e58c" />')

# Turret arms
for i in range(4):
    angle = i * 90
    rad = math.radians(angle)
    ax1 = cx + 20 * math.cos(rad)
    ay1 = cy + 20 * math.sin(rad)
    ax2 = cx + 130 * math.cos(rad)
    ay2 = cy + 130 * math.sin(rad)
    svg.append(f'<line x1="{ax1}" y1="{ay1}" x2="{ax2}" y2="{ay2}" stroke="url(#gold)" stroke-width="12" stroke-linecap="round"/>')
    svg.append(f'<circle cx="{ax2}" cy="{ay2}" r="8" fill="#fff1a0" />')

svg.append('</svg>')

with open('frontend/public/roulette_wheel_perfect.svg', 'w') as f:
    f.write('\n'.join(svg))

print("SVG generated successfully at frontend/public/roulette_wheel_perfect.svg")
