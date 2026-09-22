import type { MainModule, ShuffleConfig } from '@/assets/wasm/alloc_algo';
import type { BuddyPairPosition, ImportedConstraint } from '@/utils/JSONTypes.ts';
import { isBoolean } from 'lodash-es';

const applyBuddyGroups = (
  wasmModule: MainModule,
  cfg: ShuffleConfig,
  buddyGroups: unknown,
): void => {
  if (!Array.isArray(buddyGroups)) return;
  if (buddyGroups.length === 0) return;
  if (buddyGroups.length !== 2) {
    console.warn('buddyGroups 需為 [groupA, groupB] 或 []', buddyGroups);
    return;
  }
  const [groupA, groupB] = buddyGroups;
  if (!Array.isArray(groupA) || !Array.isArray(groupB)) {
    console.warn('buddyGroups 兩組均需為字串陣列', buddyGroups);
    return;
  }
  const vecA = new wasmModule.StringVector();
  const vecB = new wasmModule.StringVector();
  try {
    for (const name of groupA) if (typeof name === 'string') vecA.push_back(name);
    for (const name of groupB) if (typeof name === 'string') vecB.push_back(name);
    cfg.setBuddyGroups(vecA, vecB);
  } catch (e) {
    console.warn('Applying buddyGroups failed', e);
  } finally {
    vecA.delete();
    vecB.delete();
  }
};

// Build a WASM ShuffleConfig instance from a constraints JSON string.
// Returns null when wasmModule is not available.
export const buildWasmConfigFromJson = (wasmModule: MainModule | null, json: string) => {
  if (!wasmModule) return null;

  const cfg = new wasmModule.ShuffleConfig();

  let o: ImportedConstraint | null;
  try {
    o = JSON.parse(json) as ImportedConstraint;
  } catch (e) {
    console.warn('Failed to parse constraints JSON', e);
    return cfg;
  }
  if (!o) return cfg;

  try {
    if (isBoolean(o.allowFixedPoints)) cfg.setAllowFixedPoints(o.allowFixedPoints);
    if (isBoolean(o.allowOriginalNeighbors))
      cfg.setAllowOriginalNeighbors(o.allowOriginalNeighbors);
    if (isBoolean(o.diagonalsAreNeighbors)) cfg.setDiagonalsAreNeighbors(o.diagonalsAreNeighbors);
    if (isBoolean(o.crossAisleAreNeighbors))
      cfg.setCrossAisleAreNeighbors(o.crossAisleAreNeighbors);
    if (isBoolean(o.enableBuddyMatching)) cfg.setEnableBuddyMatching(o.enableBuddyMatching);
    if (isBoolean(o.doBuddyRotate)) cfg.setDoBuddyRotate(o.doBuddyRotate);
    if (
      o.prioritizeBuddyPairPosition === 'LeftAndRight' ||
      o.prioritizeBuddyPairPosition === 'FrontAndBack' ||
      o.prioritizeBuddyPairPosition === 'AllAreAcceptable'
    ) {
      cfg.setPrioritizeBuddyPairPosition(o.prioritizeBuddyPairPosition satisfies BuddyPairPosition);
    }

    if (Array.isArray(o.customForbiddenPairs)) {
      for (const p of o.customForbiddenPairs) {
        if (Array.isArray(p) && p.length >= 2) {
          try {
            cfg.addForbiddenPair(String(p[0]), String(p[1]));
          } catch (e) {
            console.warn('addForbiddenPair failed', e);
          }
        }
      }
    }

    if (Array.isArray(o.constraints)) {
      for (const c of o.constraints) {
        if (!c) continue;
        try {
          switch (c.type.toUpperCase()) {
            case 'FORCEROW':
              cfg.forceRow(String(c.name), Number(c.rowIdx));
              console.debug(`Found forceRow: name: ${c.name}, rowIdx: ${c.rowIdx}`);
              break;
            case 'FORBIDROW':
              cfg.forbidRow(String(c.name), Number(c.rowIdx));
              console.debug(`Found forbidRow: name: ${c.name}, rowIdx: ${c.rowIdx}`);
              break;
            case 'FORCECOL':
              cfg.forceCol(String(c.name), Number(c.colIdx));
              console.debug(`Found forceCol: name: ${c.name}, colIdx: ${c.colIdx}`);
              break;
            case 'FORBIDCOL':
              cfg.forbidCol(String(c.name), Number(c.colIdx));
              console.debug(`Found forbidCol: name: ${c.name}, colIdx: ${c.colIdx}`);
              break;
            case 'FORBIDSHAREROW':
              cfg.forbidShareRow(String(c.name1), String(c.name2));
              console.debug(`Found forbidShareRow: name1: ${c.name1}, name2: ${c.name2}`);
              break;
            case 'FORBIDSHARECOL':
              cfg.forbidShareCol(String(c.name1), String(c.name2));
              console.debug(`Found forbidShareCol: name1: ${c.name1}, name2: ${c.name2}`);
              break;
            default:
              console.warn('Unknown constraint type', c.type);
              break;
          }
        } catch (e) {
          console.warn('Applying constraint failed', e);
        }
      }
    }
    applyBuddyGroups(wasmModule, cfg, o.buddyGroups);
  } catch (e) {
    console.warn('Failed to build WASM config from JSON', e);
  }

  return cfg;
};
