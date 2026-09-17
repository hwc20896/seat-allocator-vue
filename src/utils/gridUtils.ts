export function extractDisplayName(raw: string): string {
  const firstLine = raw.split(/\r\n|\r|\n/).find((line) => line.trim() !== '');
  return firstLine ? firstLine.trim() : raw.trim();
}
