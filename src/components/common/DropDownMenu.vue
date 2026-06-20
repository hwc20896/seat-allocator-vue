<template>
  <div class="menu-item">
    <button class="menu-trigger">{{ label }}</button>
    <div class="dropdown-menu">
      <slot></slot>
    </div>
  </div>
</template>

<script setup lang="ts">
defineProps<{
  label: string
}>()
</script>

<style scoped>
.menu-item {
  position: relative;
  margin-right: 8px;
}

.menu-trigger {
  background: none;
  border: none;
  padding: 8px 14px;
  cursor: pointer;
  font-size: 13px;
  font-weight: 500;
  color: var(--text-muted);
  border-radius: 8px;
  transition: var(--transition-fast);
  position: relative;
}

.menu-trigger:hover {
  background-color: var(--bg-control-hover);
  color: var(--text-dark);
  transform: translateY(-1px);
}

.menu-trigger::after {
  content: '';
  position: absolute;
  bottom: 2px;
  left: 50%;
  width: 0;
  height: 2px;
  background: var(--primary);
  transition: var(--transition-fast);
  transform: translateX(-50%);
}

.menu-trigger:hover::after {
  width: 60%;
}

.dropdown-menu {
  display: none;
  position: absolute;
  top: calc(100% + 6px);
  left: 0;
  background-color: #ffffff;
  border: 1px solid var(--border-light);
  box-shadow: var(--shadow-lg);
  z-index: 100;
  min-width: 220px;
  border-radius: 10px;
  overflow: hidden;
  padding: 6px;
  backdrop-filter: blur(12px);
}

.menu-item:hover .dropdown-menu {
  display: block;
  animation: slideDown var(--transition-fast);
}

@keyframes slideDown {
  from {
    opacity: 0;
    transform: translateY(-2px);
  }
  to {
    opacity: 1;
    transform: translateY(0);
  }
}
</style>
