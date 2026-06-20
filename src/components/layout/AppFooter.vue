<template>
  <footer class="statusbar">
    <div class="status-content">
      <span class="status-indicator" :class="{ active: isGridLoaded }"></span>
      <span class="status-text">{{ statusText }}</span>
    </div>
  </footer>
</template>

<script setup lang="ts">
defineProps<{
  statusText: string
  isGridLoaded: boolean
}>()
</script>

<style scoped>
.statusbar {
  background: linear-gradient(to top, var(--bg-page) 0%, #ffffff 100%);
  border-top: 1px solid var(--border-light);
  padding: 10px 20px;
  font-size: 12px;
  color: var(--text-muted);
  flex-shrink: 0;
  box-shadow: 0 -2px 8px rgba(0, 0, 0, 0.03);
}

.status-content {
  display: flex;
  align-items: center;
  gap: 8px;
}

.status-indicator {
  width: 7px;
  height: 7px;
  border-radius: 50%;
  background-color: var(--border-medium);
  transition: var(--transition-fast);
}

.status-indicator.active {
  background: linear-gradient(135deg, #10b981 0%, #34d399 100%);
  box-shadow:
    0 0 10px rgba(16, 185, 129, 0.6),
    0 0 20px rgba(16, 185, 129, 0.3);
  animation: status-pulse 2s ease-in-out infinite;
}

@keyframes status-pulse {
  0%,
  100% {
    opacity: 1;
  }
  50% {
    opacity: 0.7;
  }
}

.status-text {
  font-weight: 500;
  letter-spacing: 0.3px;
}
</style>
