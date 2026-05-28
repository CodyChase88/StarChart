from astroquery.simbad import Simbad
from astropy.coordinates import SkyCoord
import astropy.units as u
import csv
import re

# Create Simbad instance
simbad = Simbad()

# Query stars (mas): ly -> mas: 1000 * (1 / (ly/3.26156))
query = """
SELECT main_id, ra, dec, plx_value, sp_type
FROM basic
WHERE plx_value > 67
AND otype NOT IN ('pl', 'SB*', 'El*', 'RS*')
"""

# 217 mas is ~ 15 light-years (107 stars)
# 67 mas is ~ 50 light-years (1.613 stars)
# 43.5 mas is ~ 75 light-years (5,221 stars)
# 32.6156 mas is ~ 100 light-years (10,000+ stars)
# 26 max is ~ 125 light-years (10,000 + stars)

result = simbad.query_tap(query)  # Already an astropy.table.Table

with open("stars_query.csv", "w", newline="") as f:
    writer = csv.writer(f)

    seen_coords = set()
    for row in result:  # iterate over astropy.table.Table directly
        raw_name = row["main_id"]

        # Clean up the star name
        name = raw_name.strip()  # remove leading/trailing spaces
        if name.startswith("NAME "):  # remove 'NAME ' prefix
            name = name[5:]
        name = re.sub(r"\s+", " ", name)  # replace multiple spaces with a single space
        
        print(f"{name}: {row['sp_type']}")

        # Skip explanets and companions
        if (re.search(r'\s[b-z]$', name) or # space + lowercase: "Proxima Centauri b"
            re.search(r'\s[B-D]$', name) or # space + uppercase: "HD 1326B"
            re.search(r'\d[b-z]$', name) or # digit + lowercase: "HD 1326c"
            re.search(r'\d[A-D]$', name)):  # digit + uppercase: "HD 1326B"
            continue

        # Extract RA/Dec in degrees
        ra_deg = row["ra"]
        dec_deg = row["dec"]
        plx = row["plx_value"]  # in milliarcseconds

        # Skip duplicate coordinates
        coord_key = (round(ra_deg, 4), round(dec_deg, 4))
        if coord_key in seen_coords:
            continue
        seen_coords.add(coord_key)

        # Convert RA/Dec to H:M:S / D:M:S using Astropy
        coord = SkyCoord(ra=ra_deg*u.degree, dec=dec_deg*u.degree)
        ra_hms = coord.ra.hms
        dec_dms = coord.dec.dms

        ra_parts = [int(ra_hms.h), int(ra_hms.m), round(ra_hms.s, 5)]
        dec_parts = [int(dec_dms.d), int(abs(dec_dms.m)), round(abs(dec_dms.s), 5)]

        # Convert parallax to distance in light-years
        dist_pc = 1000.0 / plx
        dist_ly = dist_pc * 3.26156

        # Write cleaned row to CSV
        writer.writerow([name] + ra_parts + dec_parts + [f"{dist_ly:.6f}"] + [row['sp_type']])
print(f"Stars written: {len(seen_coords)}")
