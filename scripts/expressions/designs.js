// SPDX-License-Identifier: MIT
// Original 128x32 pixel drawings inspired by common kaomoji.
const faces = [
  { id: 'idle' },
  { id: 'happy' },
  { id: 'laugh' },
  { id: 'wink' },
  { id: 'curious' },
  { id: 'thinking' },
  { id: 'surprise' },
  { id: 'smug' },
  { id: 'sleepy' },
  { id: 'sleep' },
  { id: 'sad' },
  { id: 'cry' },
  { id: 'angry' },
  { id: 'panic' },
  { id: 'shy' },
  { id: 'love' },
];

// Rasterize the approved balanced design into exactly 4096 one-bit pixels.
// No fonts, gray pixels, antialiasing, display rotation, or device access.
function rasterFace(id, t = 0) {
  const pixels = new Uint8Array(128 * 32);

  function putPixel(x, y, value = 1) {
    x = Math.round(x);
    y = Math.round(y);
    if (x >= 0 && x < 128 && y >= 0 && y < 32) pixels[y * 128 + x] = value;
  }

  function fillRect(x, y, width, height, value = 1) {
    for (let row = Math.round(y); row < Math.round(y + height); row++) {
      for (let column = Math.round(x); column < Math.round(x + width); column++) {
        putPixel(column, row, value);
      }
    }
  }

  function drawLine(x0, y0, x1, y1, width = 2, value = 1) {
    x0 = Math.round(x0);
    y0 = Math.round(y0);
    x1 = Math.round(x1);
    y1 = Math.round(y1);
    const dx = Math.abs(x1 - x0);
    const stepX = x0 < x1 ? 1 : -1;
    const dy = -Math.abs(y1 - y0);
    const stepY = y0 < y1 ? 1 : -1;
    let error = dx + dy;
    const halfWidth = Math.floor((width - 1) / 2);

    for (let step = 0; step < 300; step++) {
      fillRect(x0 - halfWidth, y0 - halfWidth, width, width, value);
      if (x0 === x1 && y0 === y1) break;
      const doubledError = 2 * error;
      if (doubledError >= dy) {
        error += dy;
        x0 += stepX;
      }
      if (doubledError <= dx) {
        error += dx;
        y0 += stepY;
      }
    }
  }

  function drawPath(points, width = 2, value = 1) {
    for (let i = 1; i < points.length; i++) {
      drawLine(...points[i - 1], ...points[i], width, value);
    }
  }

  function fillOval(centerX, centerY, radiusX, radiusY, value = 1) {
    for (let y = Math.floor(centerY - radiusY); y <= Math.ceil(centerY + radiusY); y++) {
      for (let x = Math.floor(centerX - radiusX); x <= Math.ceil(centerX + radiusX); x++) {
        if (((x - centerX) / radiusX) ** 2 + ((y - centerY) / radiusY) ** 2 <= 1.05) {
          putPixel(x, y, value);
        }
      }
    }
  }

  function roundedRect(centerX, centerY, width, height, radius = 3) {
    const x = Math.round(centerX - width / 2);
    const y = Math.round(centerY - height / 2);
    radius = Math.min(radius, Math.floor(height / 2), Math.floor(width / 2));
    fillRect(x + radius, y, width - radius * 2, height);
    fillRect(x, y + radius, width, height - radius * 2);
    if (radius) {
      for (const cornerX of [x + radius, x + width - radius - 1]) {
        for (const cornerY of [y + radius, y + height - radius - 1]) {
          fillOval(cornerX, cornerY, radius, radius);
        }
      }
    }
  }

  function quadraticCurve(x0, y0, controlX, controlY, x1, y1, width = 2) {
    let previous = [x0, y0];
    for (let i = 1; i <= 24; i++) {
      const u = i / 24;
      const v = 1 - u;
      const next = [
        v * v * x0 + 2 * v * u * controlX + u * u * x1,
        v * v * y0 + 2 * v * u * controlY + u * u * y1,
      ];
      drawLine(...previous, ...next, width);
      previous = next;
    }
  }

  const bounce = Math.round(Math.sin(Math.PI * Math.min(1, t / 0.8)) * -2);
  const sideOffset = t ? Math.round(Math.sin(t * 2.1) * 2) : 0;
  const blinkPhase = t % 2.4;
  let opening = 1;
  if (t > 0 && blinkPhase > 1.3 && blinkPhase < 1.62) {
    opening = Math.max(0.12, Math.abs(blinkPhase - 1.46) / 0.16);
  }

  const leftX = 34;
  const rightX = 94;
  const eyeY = 13;

  function eye(x, width = 18, height = 16, y = eyeY) {
    roundedRect(x, y, width, Math.max(2, Math.round(height * opening)), 3);
  }

  function arcEye(x, y = 14, width = 18) {
    quadraticCurve(x - width / 2, y, x, y - 14, x + width / 2, y, 2);
  }

  function closedEye(x, y = 15) {
    quadraticCurve(x - 9, y - 2, x, y + 4, x + 9, y - 2, 2);
  }

  function heart(x, y, scale = 1) {
    const outline = [[0, 2], [2, 0], [5, 0], [7, 2], [9, 0], [12, 0], [14, 2], [14, 6], [7, 13], [0, 6]];
    for (let row = 0; row < 14; row++) {
      for (let column = 0; column < 15; column++) {
        let inside = false;
        for (let i = 0, j = outline.length - 1; i < outline.length; j = i++) {
          const [x0, y0] = outline[i];
          const [x1, y1] = outline[j];
          if ((y0 > row) !== (y1 > row) && column < (x1 - x0) * (row - y0) / (y1 - y0) + x0) {
            inside = !inside;
          }
        }
        if (inside) fillRect(x + (column - 7) * scale, y + (row - 6) * scale, scale, scale);
      }
    }
  }

  function star(x, y, radius = 4) {
    drawLine(x - radius, y, x + radius, y, 1);
    drawLine(x, y - radius, x, y + radius, 1);
    putPixel(x - 1, y - 1);
    putPixel(x + 1, y + 1);
  }

  function drop(x, y) {
    drawPath([[x, y - 3], [x - 2, y], [x - 2, y + 3], [x, y + 4], [x + 2, y + 3], [x + 2, y], [x, y - 3]], 1);
    fillRect(x - 1, y, 3, 3);
  }

  function mouth(type = 'smile') {
    switch (type) {
      case 'smile':
        quadraticCurve(59, 22, 64, 28, 69, 22, 1);
        break;
      case 'w':
        drawPath([[58, 23], [60, 26], [63, 26], [64, 24], [65, 26], [68, 26], [70, 23]], 1);
        break;
      case 'flat':
        drawLine(60, 25, 68, 25, 1);
        break;
      case 'o':
        fillOval(64, 24, 3, 4);
        fillOval(64, 24, 1, 2, 0);
        break;
      case 'open':
        drawPath([[59, 22], [69, 22], [68, 26], [66, 28], [62, 28], [60, 26], [59, 22]], 1);
        fillRect(61, 23, 7, 3);
        break;
      case 'sad':
        quadraticCurve(59, 27, 64, 20, 69, 27, 1);
        break;
      case 'wave':
        drawPath([[58, 25], [61, 23], [64, 25], [67, 23], [70, 25]], 1);
        break;
    }
  }

  switch (id) {
    case 'idle':
      eye(leftX + sideOffset);
      eye(rightX + sideOffset);
      mouth();
      break;

    case 'happy':
      arcEye(leftX, 15 + bounce);
      arcEye(rightX, 15 + bounce);
      mouth('w');
      break;

    case 'laugh':
      drawPath([[leftX - 7, 7], [leftX + 2, 14], [leftX - 7, 20]], 3);
      drawPath([[rightX + 7, 7], [rightX - 2, 14], [rightX + 7, 20]], 3);
      mouth(t && Math.sin(t * 9) < 0 ? 'smile' : 'open');
      break;

    case 'wink':
      eye(leftX);
      if (!t || t % 2 < 1.25) {
        drawPath([[rightX + 8, 8], [rightX - 3, 14], [rightX + 8, 18]], 2);
      } else {
        eye(rightX);
      }
      mouth();
      if (!t || t % 2 < 1.5) star(116, 7);
      break;

    case 'curious': {
      const eyeOffset = t ? Math.round(Math.sin(t * 3) * 2) : 0;
      fillOval(leftX, eyeY + eyeOffset, 6, 6);
      fillOval(leftX, eyeY + eyeOffset, 3, 3, 0);
      fillOval(rightX, eyeY - eyeOffset, 10, 10);
      fillOval(rightX, eyeY - eyeOffset, 6, 6, 0);
      mouth('flat');
      drawPath([[112, 4], [114, 2], [117, 2], [119, 4], [119, 6], [115, 9], [115, 11]], 1);
      fillRect(115, 14, 2, 2);
      break;
    }

    case 'thinking': {
      roundedRect(leftX + 3, eyeY + 1, 19, 7, 2);
      eye(rightX + 3, 14, 12, eyeY);
      drawLine(leftX - 9, 6, leftX + 10, 6, 2);
      mouth('flat');
      const dotCount = t ? 1 + Math.floor(t * 2) % 3 : 3;
      for (let i = 0; i < dotCount; i++) fillRect(110 + i * 4, 25, 2, 2);
      break;
    }

    case 'surprise': {
      const expansion = t ? Math.round(Math.max(0, 1 - t / 0.8) * 2) : 0;
      for (const x of [leftX, rightX]) {
        fillOval(x, eyeY, 9 + expansion, 10 + expansion);
        fillOval(x, eyeY, 5, 6, 0);
      }
      mouth('o');
      break;
    }

    case 'smug': {
      for (const x of [leftX, rightX]) {
        roundedRect(x, eyeY + 3, 20, 10, 2);
        fillRect(x - 11, eyeY - 2, 23, 5, 0);
        fillRect(x + 3, eyeY + 3, 3, 4, 0);
      }
      const eyebrowOffset = t ? Math.round(Math.sin(t * 3) * 2) : 0;
      drawLine(leftX - 9, 7, leftX + 8, 5 + eyebrowOffset, 2);
      drawLine(rightX - 9, 7, rightX + 8, 7, 2);
      mouth();
      break;
    }

    case 'sleepy':
      for (const x of [leftX, rightX]) {
        const height = t ? Math.max(2, 8 - Math.round(t * 2)) : 5;
        roundedRect(x, eyeY + 3, 18, height, 1);
        drawLine(x - 9, 11, x + 8, 11, 2);
      }
      mouth(t > 0.6 && t < 1.8 ? 'o' : 'flat');
      break;

    case 'sleep': {
      const breathingOffset = t ? Math.round(Math.sin(t * 2)) : 0;
      closedEye(leftX, 15 + breathingOffset);
      closedEye(rightX, 15 + breathingOffset);
      mouth('w');
      const symbolOffset = t ? -Math.floor(t * 1.5) % 3 : 0;
      drawPath([[109, 13 + symbolOffset], [114, 13 + symbolOffset], [109, 18 + symbolOffset], [114, 18 + symbolOffset]], 1);
      drawPath([[117, 4 + symbolOffset], [123, 4 + symbolOffset], [117, 10 + symbolOffset], [123, 10 + symbolOffset]], 1);
      break;
    }

    case 'sad':
      for (const x of [leftX, rightX]) {
        fillOval(x, eyeY + 3, 7, 8);
        fillRect(x - 5, eyeY + 2, 2, 4, 0);
      }
      drawLine(leftX - 9, 5, leftX + 6, 2, 2);
      drawLine(rightX - 6, 2, rightX + 9, 5, 2);
      drop(leftX - 8, 23 + (t ? Math.round(Math.sin(t * 3)) : 0));
      mouth('sad');
      break;

    case 'cry':
      for (const x of [leftX, rightX]) {
        drawLine(x - 9, 8, x + 9, 8, 3);
        fillRect(x - 5, 10, 3, 17);
        fillRect(x + 3, 10, 3, 17);
        if (t) {
          const tearOffset = Math.floor(t * 7) % 13;
          fillRect(x - 5, 12 + tearOffset, 3, 2, 0);
          fillRect(x + 3, 11 + (tearOffset + 6) % 13, 3, 2, 0);
        }
      }
      mouth('wave');
      break;

    case 'angry':
      eye(leftX, 18, 12, 16);
      eye(rightX, 18, 12, 16);
      drawPath([[leftX - 12, 5], [leftX + 10, 12]], 3);
      drawPath([[rightX - 10, 12], [rightX + 12, 5]], 3);
      mouth('sad');
      if (!t || t % 1.1 < 0.7) {
        drawPath([[114, 3], [114, 7], [111, 7]], 1);
        drawPath([[118, 3], [118, 7], [121, 7]], 1);
        drawPath([[114, 14], [114, 10], [111, 10]], 1);
        drawPath([[118, 14], [118, 10], [121, 10]], 1);
      }
      break;

    case 'panic':
      drawPath([[leftX - 6, 7], [leftX + 4, 14], [leftX - 6, 21]], 2);
      drawPath([[rightX + 6, 7], [rightX - 4, 14], [rightX + 6, 21]], 2);
      mouth('wave');
      drop(116, 10 + (t ? Math.floor(t * 5) % 8 : 0));
      break;

    case 'shy':
      closedEye(leftX, 13);
      closedEye(rightX, 13);
      mouth('w');
      if (!t || t % 1.5 < 1.1) {
        for (const x of [19, 24, 101, 106]) drawLine(x, 20, x - 2, 24, 1);
      }
      break;

    case 'love': {
      const heartOffset = t ? Math.round(Math.sin(t * 7)) : 0;
      heart(leftX, eyeY + heartOffset);
      heart(rightX, eyeY + heartOffset);
      mouth('w');
      if (!t || t % 1 < 0.5) {
        star(14, 11, 2);
        star(113, 22, 2);
      }
      break;
    }
  }

  return pixels;
}

export { faces, rasterFace };
