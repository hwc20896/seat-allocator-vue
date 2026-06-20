<template>
  <div class="bottom-layout">
    <button
      class="shuffle-button"
      :class="{ 'is-active-shuffling': isShuffling }"
      :disabled="!isGridLoaded || isShuffling"
      @click="$emit('shuffle')"
    >
      <span v-if="isShuffling" class="spinner"></span>
      {{ isShuffling ? '正在隨機分配洗牌...' : '開始隨機分配！' }}
    </button>
  </div>
</template>

<script setup lang="ts">
defineProps<{
  isShuffling: boolean
  isGridLoaded: boolean
}>()

defineEmits<{
  shuffle: []
}>()
</script>

<style scoped>
.bottom-layout {
  display: flex;
  justify-content: center;
  align-items: center;
  flex-shrink: 0;
  padding: 4px 0;
}

.shuffle-button {
  width: 340px;
  height: 56px;
  font-size: 17px;
  font-weight: 700;
  background: linear-gradient(135deg, var(--primary) 0%, #8b5cf6 50%, var(--primary-hover) 100%);
  background-size: 200% 200%;
  color: #ffffff;
  border: none;
  border-radius: 12px;
  cursor: pointer;
  box-shadow:
    0 6px 20px rgba(99, 102, 241, 0.35),
    0 0 0 1px rgba(99, 102, 241, 0.1);
  transition: var(--transition-normal);
  display: flex;
  align-items: center;
  justify-content: center;
  gap: 12px;
  letter-spacing: 0.5px;
  position: relative;
  overflow: hidden;
}

.shuffle-button::before {
  content: '';
  position: absolute;
  top: 0;
  left: -100%;
  width: 100%;
  height: 100%;
  background: linear-gradient(90deg, transparent, rgba(255, 255, 255, 0.2), transparent);
  transition: 0.5s;
}

.shuffle-button:hover:not(:disabled)::before {
  left: 100%;
}

.shuffle-button:hover:not(:disabled) {
  box-shadow:
    0 8px 28px rgba(99, 102, 241, 0.45),
    0 0 0 2px rgba(99, 102, 241, 0.15);
  transform: translateY(-2px);
  background-position: 100% 0;
}

.shuffle-button:active:not(:disabled) {
  transform: translateY(0);
  box-shadow: 0 4px 16px rgba(99, 102, 241, 0.3);
}

.shuffle-button:disabled {
  background: #cbd5e1;
  color: #64748b;
  cursor: not-allowed;
  box-shadow: none;
}

/* Spinner animation */
.spinner {
  width: 20px;
  height: 20px;
  border: 3px solid rgba(255, 255, 255, 0.3);
  border-top-color: #ffffff;
  border-radius: 50%;
  animation: rotate 0.7s linear infinite;
  box-shadow: 0 0 10px rgba(255, 255, 255, 0.3);
}

@keyframes rotate {
  to {
    transform: rotate(360deg);
  }
}
</style>
